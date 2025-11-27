#include <CadPL/PipelineLibrary.h>
#include <CadR/VulkanDevice.h>
#include <memory>

using namespace std;
using namespace CadPL;


// pipeline vertex input
static constexpr const vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo(
	vk::PipelineVertexInputStateCreateFlags(),  // flags
	0,  // vertexBindingDescriptionCount
	nullptr,  // pVertexBindingDescriptions
	0,  // vertexAttributeDescriptionCount
	nullptr  // pVertexAttributeDescriptions
);

// pipeline empty viewport state
static constexpr const vk::PipelineViewportStateCreateInfo pipelineEmptyViewportState{
	vk::PipelineViewportStateCreateFlags(),  // flags
	1,  // viewportCount
	nullptr,  // pViewports
	1,  // scissorCount
	nullptr  // pScissors
};




PipelineLibrary::~PipelineLibrary()
{
	assert(_pipelineFamilyMap.empty() && "PipelineLibrary::~PipelineLibrary(): All pipelines "
		"owned by PipelineLibrary must be released before destroying PipelineLibrary.");
}


PipelineFamily::~PipelineFamily()
{
	assert(_pipelineMap.empty() && "PipelineFamily::~PipelineFamily(): All SharedPipelines must be released before destroying PipelineFamily or PipelineLibrary.");
}


PipelineFamily::PipelineFamily(PipelineLibrary& pipelineLibrary) noexcept
	: _device(pipelineLibrary._device)
	, _pipelineLibrary(&pipelineLibrary)
{
}


void PipelineFamily::destroyPipeline(void* pipelineObject) noexcept
{
	PipelineObject* po = reinterpret_cast<PipelineObject*>(pipelineObject); 
	PipelineFamily* pf = po->pipelineFamily;
	po->cadrPipeline.destroyPipeline(*pf->_device);
	pf->_pipelineMap.erase(po->mapIterator);

	if(pf->_pipelineMap.empty())
		pf->_pipelineLibrary->_pipelineFamilyMap.erase(pf->_eraseIt);
}


SharedPipeline PipelineFamily::getOrCreatePipeline(const PipelineState& pipelineState)
{
	auto [it, newRecord] = _pipelineMap.try_emplace(pipelineState);
	if(newRecord) {
		try {
			it->second.cadrPipeline.init(
				createPipeline(pipelineState),
				_pipelineLibrary->pipelineLayout(),
				_pipelineLibrary->descriptorSetLayoutList()
			);
		} catch(...) {
			_pipelineMap.erase(it);
			throw;
		}
		it->second.referenceCounter = 0;
		it->second.pipelineFamily = this;
		it->second.mapIterator = it;
	}
	return SharedPipeline(&it->second);
}


SharedPipeline PipelineLibrary::getOrCreatePipeline(const ShaderState& shaderState, const PipelineState& pipelineState)
{
	auto [it, newRecord] = _pipelineFamilyMap.try_emplace(shaderState, *this);
	if(newRecord) {
		try {
			it->second._vertexShader = _shaderLibrary->getOrCreateVertexShader(shaderState);
			it->second._geometryShader = _shaderLibrary->getOrCreateGeometryShader(shaderState);
			it->second._fragmentShader = _shaderLibrary->getOrCreateFragmentShader(shaderState);
		} catch(...) {
			_pipelineFamilyMap.erase(it);
			throw;
		}
		it->second._eraseIt = it;
		it->second._primitiveTopology = shaderState.primitiveTopology;
	}
	return it->second.getOrCreatePipeline(pipelineState);
}


vk::Pipeline PipelineFamily::createPipeline(const PipelineState& pipelineState)
{
	if(pipelineState.viewportAndScissorHandling == PipelineState::ViewportAndScissorHandling::SetFunction)
		if(_pipelineLibrary->_viewportList.size() == 0 || _pipelineLibrary->_scissorList.size() == 0)
			return {};

	array<vk::PipelineShaderStageCreateInfo, 3> shaderStages{
		vk::PipelineShaderStageCreateInfo{
			vk::PipelineShaderStageCreateFlags(),  // flags
			vk::ShaderStageFlagBits::eVertex,  // stage
			_vertexShader,  // module
			"main",  // pName
			nullptr,  // pSpecializationInfo
		},
		vk::PipelineShaderStageCreateInfo{
			vk::PipelineShaderStageCreateFlags(),  // flags
			vk::ShaderStageFlagBits::eFragment,  // stage
			_fragmentShader,  // module
			"main",  // pName
			nullptr,  // pSpecializationInfo
		},
		vk::PipelineShaderStageCreateInfo{
			vk::PipelineShaderStageCreateFlags(),  // flags
			vk::ShaderStageFlagBits::eGeometry,  // stage
			_geometryShader,  // module
			"main",  // pName
			nullptr,  // pSpecializationInfo
		},
	};
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState{
		vk::PipelineInputAssemblyStateCreateFlags(),  // flags
		_primitiveTopology,  // topology
		VK_FALSE  // primitiveRestartEnable
	};
	vk::PipelineViewportStateCreateInfo viewportState{
		vk::PipelineViewportStateCreateFlags(),  // flags
		1,  // viewportCount
		&pipelineState.viewport,  // pViewports
		1,  // scissorCount
		&pipelineState.scissor  // pScissors
	};
	vk::PipelineRasterizationStateCreateInfo rasterizationState{
		vk::PipelineRasterizationStateCreateFlags(),  // flags
		VK_FALSE,  // depthClampEnable
		VK_FALSE,  // rasterizerDiscardEnable
		vk::PolygonMode::eFill,  // polygonMode
		pipelineState.cullMode,  // cullMode
		pipelineState.frontFace,  // frontFace
		pipelineState.depthBiasEnable,  // depthBiasEnable
		pipelineState.depthBiasConstantFactor,  // depthBiasConstantFactor
		pipelineState.depthBiasClamp,  // depthBiasClamp
		pipelineState.depthBiasSlopeFactor,  // depthBiasSlopeFactor
		pipelineState.lineWidth  // lineWidth
	};
	vk::PipelineMultisampleStateCreateInfo multisampleState{
		vk::PipelineMultisampleStateCreateFlags(),  // flags
		pipelineState.rasterizationSamples,  // rasterizationSamples
		pipelineState.sampleShadingEnable,  // sampleShadingEnable
		pipelineState.minSampleShading,  // minSampleShading
		nullptr,   // pSampleMask
		VK_FALSE,  // alphaToCoverageEnable
		VK_FALSE   // alphaToOneEnable
	};
	vk::PipelineDepthStencilStateCreateInfo depthStencilState{
		vk::PipelineDepthStencilStateCreateFlags(),  // flags
		pipelineState.depthTestEnable,  // depthTestEnable
		pipelineState.depthWriteEnable,  // depthWriteEnable
		vk::CompareOp::eLess,  // depthCompareOp
		VK_FALSE,  // depthBoundsTestEnable
		VK_FALSE,  // stencilTestEnable
		vk::StencilOpState(),  // front
		vk::StencilOpState(),  // back
		0.f,  // minDepthBounds
		0.f   // maxDepthBounds
	};
	vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachmentStates(pipelineState.blendState.size());
	for(size_t i=0,c=pipelineState.blendState.size(); i<c; i++) {
		const auto& src = pipelineState.blendState[i];
		vk::PipelineColorBlendAttachmentState& dst = colorBlendAttachmentStates[i];
		dst.blendEnable         = src.blendEnable;
		dst.srcColorBlendFactor = src.srcColorBlendFactor;
		dst.dstColorBlendFactor = src.dstColorBlendFactor;
		dst.colorBlendOp        = src.colorBlendOp;
		dst.srcAlphaBlendFactor = src.srcAlphaBlendFactor;
		dst.dstAlphaBlendFactor = src.dstAlphaBlendFactor;
		dst.alphaBlendOp        = src.alphaBlendOp;
		dst.colorWriteMask      = src.colorWriteMask;
	};
	vk::PipelineColorBlendStateCreateInfo colorBlendState{
		vk::PipelineColorBlendStateCreateFlags(),  // flags
		VK_FALSE,  // logicOpEnable
		vk::LogicOp::eClear,  // logicOp
		uint32_t(colorBlendAttachmentStates.size()),  // attachmentCount
		colorBlendAttachmentStates.data(),  // pAttachments
		array<float,4>{0.f,0.f,0.f,0.f}  // blendConstants
	};
	vk::GraphicsPipelineCreateInfo createInfo(
		vk::PipelineCreateFlags(),  // flags
		uint32_t(shaderStages.size()),  // stageCount
		shaderStages.data(),  // pStages
		&pipelineVertexInputStateCreateInfo,  // pVertexInputState
		&inputAssemblyState,  // pInputAssemblyState
		nullptr, // pTessellationState
		&viewportState,  // pViewportState
		&rasterizationState,  // pRasterizationState
		&multisampleState,  // pMultisampleState
		&depthStencilState,  // pDepthStencilState
		&colorBlendState,  // pColorBlendState
		nullptr,  // pDynamicState
		_pipelineLibrary->_shaderLibrary->pipelineLayout(),  // layout
		pipelineState.renderPass,  // renderPass
		pipelineState.subpass,  // subpass
		nullptr,  // basePipelineHandle
		-1 // basePipelineIndex
	);

	return
		_device->createGraphicsPipeline(
			_pipelineLibrary->_pipelineCache,  // pipelineCache
			createInfo  // createInfo
		);
}


void PipelineLibrary::setViewportAndScissor(const std::vector<vk::Viewport>& viewportList,
	const std::vector<vk::Rect2D>& scissorList)
{
	// update variables
	_viewportList = viewportList;
	_scissorList = scissorList;
	if(viewportList.size() == 0 || scissorList.size() == 0)
		return;

	// get list of pipelines to be updated
	vector<map<PipelineState,PipelineFamily::PipelineObject>::iterator> pipelinesToBeUpdated;
	for(auto familyIt=_pipelineFamilyMap.begin(); familyIt!=_pipelineFamilyMap.end(); familyIt++) {
		PipelineFamily& f = familyIt->second;
		for(auto pipelineIt=f._pipelineMap.begin(); pipelineIt!=f._pipelineMap.end(); pipelineIt++)
			if(pipelineIt->first.viewportAndScissorHandling == PipelineState::ViewportAndScissorHandling::SetFunction)
				pipelinesToBeUpdated.push_back(pipelineIt);
	}
	size_t numPipelines = pipelinesToBeUpdated.size();
	if(numPipelines == 0)
		return;

	// prepare pipeline create structs
	unique_ptr<array<vk::PipelineShaderStageCreateInfo,3>[]> shaderStageList =
		make_unique<array<vk::PipelineShaderStageCreateInfo,3>[]>(numPipelines);
	vector<vk::PipelineInputAssemblyStateCreateInfo> inputAssemblyStateList;
	vector<tuple<vk::PipelineViewportStateCreateInfo,vk::Viewport,vk::Rect2D>> viewportStateList;
	vector<vk::PipelineRasterizationStateCreateInfo> rasterizationStateList;
	vector<vk::PipelineMultisampleStateCreateInfo> multisampleStateList;
	vector<vk::PipelineDepthStencilStateCreateInfo> depthStencilStateList;
	vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachmentStateList;
	vector<vk::PipelineColorBlendStateCreateInfo> colorBlendStateList;
	vector<vk::GraphicsPipelineCreateInfo> createInfoList;
	inputAssemblyStateList.reserve(numPipelines);
	viewportStateList.reserve(numPipelines);
	rasterizationStateList.reserve(numPipelines);
	multisampleStateList.reserve(numPipelines);
	depthStencilStateList.reserve(numPipelines);
	colorBlendStateList.reserve(numPipelines);
	createInfoList.resize(numPipelines);

	// maximum size of colorBlendAttachmentStateList
	colorBlendAttachmentStateList.reserve(
		[&](){
			size_t numAttachments = 0;
			for(auto it : pipelinesToBeUpdated)
				numAttachments += it->first.blendState.size();
			return numAttachments;
		}()
	);

	for(size_t i=0; i<numPipelines; i++) {

		// flags
		auto& createInfo = createInfoList[i];
		createInfo.flags = vk::PipelineCreateFlags();

		// stageCount and pStages
		const PipelineFamily& pipelineFamily = *pipelinesToBeUpdated[i]->second.pipelineFamily;
		auto& shaderStages = shaderStageList[i];
		shaderStages[0] =
			vk::PipelineShaderStageCreateInfo{
				vk::PipelineShaderStageCreateFlags(),  // flags
				vk::ShaderStageFlagBits::eVertex,  // stage
				pipelineFamily._vertexShader,  // module
				"main",  // pName
				nullptr,  // pSpecializationInfo
			};
		shaderStages[1] =
			vk::PipelineShaderStageCreateInfo{
				vk::PipelineShaderStageCreateFlags(),  // flags
				vk::ShaderStageFlagBits::eFragment,  // stage
				pipelineFamily._fragmentShader,  // module
				"main",  // pName
				nullptr,  // pSpecializationInfo
			};
		if(pipelineFamily._geometryShader) {
			shaderStages[2] =
				vk::PipelineShaderStageCreateInfo{
					vk::PipelineShaderStageCreateFlags(),  // flags
					vk::ShaderStageFlagBits::eGeometry,  // stage
					pipelineFamily._geometryShader,  // module
					"main",  // pName
					nullptr,  // pSpecializationInfo
				};
			createInfo.stageCount = 3;
		}
		else
			createInfo.stageCount = 2;
		createInfo.pStages = shaderStages.data();

		// pVertexInputState
		createInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;

		// pInputAssemblyState
		for(size_t j=0, c=inputAssemblyStateList.size(); j<c; j++)
			if(inputAssemblyStateList[j].topology == pipelineFamily._primitiveTopology) {
				createInfo.pInputAssemblyState = &inputAssemblyStateList[j];
				goto foundInputAssemblyState;
			}
		createInfo.pInputAssemblyState =
			&inputAssemblyStateList.emplace_back(
				vk::PipelineInputAssemblyStateCreateFlags(),  // flags
				pipelineFamily._primitiveTopology,  // topology
				VK_FALSE  // primitiveRestartEnable
			);
	foundInputAssemblyState:;

		// pTessellationState
		createInfo.pTessellationState = nullptr;

		// pViewportState
		vk::Viewport newViewport = viewportList.at(pipelinesToBeUpdated[i]->first.viewportAndScissorIndex);
		vk::Rect2D newScissor = scissorList.at(pipelinesToBeUpdated[i]->first.viewportAndScissorIndex);
		decltype(viewportStateList)::value_type* viewportState;
		for(size_t j=0, c=viewportStateList.size(); j<c; j++)
			if(get<1>(viewportStateList[j]) == newViewport && get<2>(viewportStateList[j]) == newScissor) {
				createInfo.pViewportState = &get<0>(viewportStateList[j]);
				goto foundViewportState;
			}
		viewportState =
			&viewportStateList.emplace_back(pipelineEmptyViewportState, newViewport, newScissor);
		get<0>(*viewportState).pViewports = &get<1>(*viewportState);
		get<0>(*viewportState).pScissors = &get<2>(*viewportState);
		createInfo.pViewportState = &get<0>(*viewportState);
	foundViewportState:;

		// pRasterizationState
		const PipelineState& pipelineState = pipelinesToBeUpdated[i]->first;
		for(size_t j=0, c=rasterizationStateList.size(); j<c; j++) {
			const auto& rasterizationState = rasterizationStateList[j];
			if(rasterizationState.cullMode == pipelineState.cullMode &&
			   rasterizationState.frontFace == pipelineState.frontFace &&
			   (rasterizationState.depthBiasEnable!=0) == pipelineState.depthBiasEnable &&
			   rasterizationState.depthBiasConstantFactor == pipelineState.depthBiasConstantFactor &&
			   rasterizationState.depthBiasClamp == pipelineState.depthBiasClamp &&
			   rasterizationState.depthBiasSlopeFactor == pipelineState.depthBiasSlopeFactor &&
			   rasterizationState.lineWidth == pipelineState.lineWidth)
			{
				createInfo.pRasterizationState = &rasterizationStateList[j];
				goto foundRasterizationState;
			}
		}
		createInfo.pRasterizationState =
			&rasterizationStateList.emplace_back(
				vk::PipelineRasterizationStateCreateInfo{
					vk::PipelineRasterizationStateCreateFlags(),  // flags
					VK_FALSE,  // depthClampEnable
					VK_FALSE,  // rasterizerDiscardEnable
					vk::PolygonMode::eFill,  // polygonMode
					pipelineState.cullMode,  // cullMode
					pipelineState.frontFace,  // frontFace
					pipelineState.depthBiasEnable,  // depthBiasEnable
					pipelineState.depthBiasConstantFactor,  // depthBiasConstantFactor
					pipelineState.depthBiasClamp,  // depthBiasClamp
					pipelineState.depthBiasSlopeFactor,  // depthBiasSlopeFactor
					pipelineState.lineWidth  // lineWidth
				}
			);
	foundRasterizationState:;

		// pMultisampleState
		for(size_t j=0, c=multisampleStateList.size(); j<c; j++) {
			const auto& multisampleState = multisampleStateList[j];
			if(multisampleState.rasterizationSamples == pipelineState.rasterizationSamples &&
			   (multisampleState.sampleShadingEnable!=0) == pipelineState.sampleShadingEnable &&
			   multisampleState.minSampleShading == pipelineState.minSampleShading)
			{
				createInfo.pMultisampleState = &multisampleStateList[j];
				goto foundMultisampleState;
			}
		}
		createInfo.pMultisampleState =
			&multisampleStateList.emplace_back(
				vk::PipelineMultisampleStateCreateInfo{
					vk::PipelineMultisampleStateCreateFlags(),  // flags
					pipelineState.rasterizationSamples,  // rasterizationSamples
					pipelineState.sampleShadingEnable,  // sampleShadingEnable
					pipelineState.minSampleShading,  // minSampleShading
					nullptr,   // pSampleMask
					VK_FALSE,  // alphaToCoverageEnable
					VK_FALSE   // alphaToOneEnable
				}
			);
	foundMultisampleState:;

		// pDepthStencilState
		for(size_t j=0, c=depthStencilStateList.size(); j<c; j++) {
			const auto& depthStencilState = depthStencilStateList[j];
			if((depthStencilState.depthTestEnable!=0) == pipelineState.depthTestEnable &&
			   (depthStencilState.depthWriteEnable!=0) == pipelineState.depthWriteEnable)
			{
				createInfo.pDepthStencilState = &depthStencilStateList[j];
				goto foundDepthStencilState;
			}
		}
		createInfo.pDepthStencilState =
			&depthStencilStateList.emplace_back(
				vk::PipelineDepthStencilStateCreateInfo{
					vk::PipelineDepthStencilStateCreateFlags(),  // flags
					pipelineState.depthTestEnable,  // depthTestEnable
					pipelineState.depthWriteEnable,  // depthWriteEnable
					vk::CompareOp::eLess,  // depthCompareOp
					VK_FALSE,  // depthBoundsTestEnable
					VK_FALSE,  // stencilTestEnable
					vk::StencilOpState(),  // front
					vk::StencilOpState(),  // back
					0.f,  // minDepthBounds
					0.f   // maxDepthBounds
				}
			);
	foundDepthStencilState:;

		// pColorBlendState
		vk::PipelineColorBlendAttachmentState* attachmentsPtr;
		for(size_t j=0, c=colorBlendStateList.size(); j<c; j++) {
			const auto& colorBlendStateDst = colorBlendStateList[j];
			if(colorBlendStateDst.attachmentCount != pipelineState.blendState.size())
				goto blendStateDiffers;
			for(size_t k=0,c=pipelineState.blendState.size(); k<c; k++) {
				const auto& blendStateSrc = pipelineState.blendState[k];
				const auto& blendStateDst = colorBlendStateDst.pAttachments[k];
				if(blendStateSrc.blendEnable==false && blendStateDst.blendEnable==false)
					continue;
				if(blendStateSrc.blendEnable != (blendStateDst.blendEnable!=0))
					goto blendStateDiffers;
				if(blendStateSrc.srcColorBlendFactor != blendStateDst.srcColorBlendFactor ||
					blendStateSrc.dstColorBlendFactor != blendStateDst.dstColorBlendFactor ||
					blendStateSrc.colorBlendOp != blendStateDst.colorBlendOp ||
					blendStateSrc.srcAlphaBlendFactor != blendStateDst.srcAlphaBlendFactor ||
					blendStateSrc.dstAlphaBlendFactor != blendStateDst.dstAlphaBlendFactor ||
					blendStateSrc.alphaBlendOp != blendStateDst.alphaBlendOp ||
					blendStateSrc.colorWriteMask != blendStateDst.colorWriteMask)
				{
					goto blendStateDiffers;
				}
			}
			createInfo.pColorBlendState = &colorBlendStateDst;
			goto foundColorBlendState;
		blendStateDiffers:;
		}
		attachmentsPtr = colorBlendAttachmentStateList.data() + colorBlendAttachmentStateList.size();
		for(size_t j=0,c=pipelineState.blendState.size(); j<c; j++) {
			const auto& src = pipelineState.blendState[j];
			colorBlendAttachmentStateList.emplace_back(
				vk::PipelineColorBlendAttachmentState{
					src.blendEnable,
					src.srcColorBlendFactor,
					src.dstColorBlendFactor,
					src.colorBlendOp,
					src.srcAlphaBlendFactor,
					src.dstAlphaBlendFactor,
					src.alphaBlendOp,
					src.colorWriteMask,
				}
			);
		};
		createInfo.pColorBlendState =
			&colorBlendStateList.emplace_back(
				vk::PipelineColorBlendStateCreateInfo(
					vk::PipelineColorBlendStateCreateFlags(),  // flags
					VK_FALSE,  // logicOpEnable
					vk::LogicOp::eClear,  // logicOp
					uint32_t(pipelineState.blendState.size()),  // attachmentCount
					attachmentsPtr,  // pAttachments
					array<float,4>{0.f,0.f,0.f,0.f}  // blendConstants
				)
			);
	foundColorBlendState:;

		// pDynamicState
		createInfo.pDynamicState = nullptr;

		// remaining createInfo members
		createInfo.layout = _shaderLibrary->pipelineLayout();
		createInfo.renderPass = pipelineState.renderPass;
		createInfo.subpass = pipelineState.subpass;
		createInfo.basePipelineHandle = nullptr;
		createInfo.basePipelineIndex = -1;

	}

	// create pipelines
	vector<vk::Pipeline> pipelineList(numPipelines);
	VkResult r =
		_device->vkCreateGraphicsPipelines(
			_device->handle(),  // device
			_pipelineCache,  // pipelineCache
			uint32_t(numPipelines),  // createInfoCount
			reinterpret_cast<VkGraphicsPipelineCreateInfo*>(createInfoList.data()),  // pCreateInfos
			nullptr,  // pAllocator
			reinterpret_cast<VkPipeline*>(pipelineList.data())  // pPipelines
		);
	if(r != VK_SUCCESS) {
		for(vk::Pipeline p : pipelineList)
			_device->destroy(p);
		pipelineList.clear();
	#if VK_HEADER_VERSION < 256  // throwResultException moved to detail namespace on 2023-06-28 and the change went public in 1.3.256
		vk::throwResultException(vk::Result(r), "vk::Device::createGraphicsPipelines");
	#else
		vk::detail::throwResultException(vk::Result(r), "vk::Device::createGraphicsPipelines");
	#endif
	}

	// update pipelines inside SharedPipeline object
	for(size_t i=0, c=pipelinesToBeUpdated.size(); i<c; i++)
		pipelinesToBeUpdated[i]->second.cadrPipeline.set(pipelineList[i]);
}
