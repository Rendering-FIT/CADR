#include <CadPL/PipelineLibrary.h>
#include <CadR/VulkanDevice.h>

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

// specialization data map
static constexpr const std::array specializationMap {
	vk::SpecializationMapEntry{0,0,4},  // constantID, offset, size
	vk::SpecializationMapEntry{1,4,4},
	vk::SpecializationMapEntry{2,8,4},
	vk::SpecializationMapEntry{3,12,4},
	vk::SpecializationMapEntry{4,16,4},
	vk::SpecializationMapEntry{5,20,4},
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


void PipelineFamily::destroyPipeline(void* pipelineObject) noexcept
{
	PipelineObject* po = reinterpret_cast<PipelineObject*>(pipelineObject); 
	PipelineFamily* pf = po->pipelineFamily;
	po->cadrPipeline.destroyPipeline(*pf->_device);
	pf->_pipelineMap.erase(po->mapIterator);

	if(pf->_pipelineMap.empty())
		pf->_pipelineLibrary->_pipelineFamilyMap.erase(pf->_mapIterator);
}


SharedPipeline PipelineFamily::getOrCreatePipeline(const PipelineState& pipelineState)
{
	auto [it, newRecord] = _pipelineMap.try_emplace(pipelineState);
	if(!newRecord)
		return SharedPipeline(&it->second);

	// initialize new record
	// (do not throw until SharedPipeline is created)
	it->second.cadrPipeline.init(
		nullptr,
		_pipelineLibrary->pipelineLayout(),
		&_pipelineLibrary->descriptorSetLayoutList()
	);
	it->second.referenceCounter = 0;
	it->second.pipelineFamily = this;
	it->second.mapIterator = it;
	SharedPipeline sharedPipeline(&it->second);

	// if viewport, scissor or projection matrix were not set yet
	// and they are required by the pipeline, skip pipeline creation;
	// (the pipeline will be created in setProjectionViewportAndScissor() or similar function)
	if(pipelineState.viewportAndScissorHandling == PipelineState::ViewportAndScissorHandling::SetFunction &&
		(_pipelineLibrary->_viewportList.empty() || _pipelineLibrary->_scissorList.empty()))
			return sharedPipeline;
	if(_mapIterator->first.projectionHandling == ShaderState::ProjectionHandling::PerspectivePushAndSpecializationConstants &&
		_pipelineLibrary->_specializationData.empty())
			return sharedPipeline;

	// create pipeline
	PipelineLibrary::CreationDataSet creationDataSet(*_pipelineLibrary);
	creationDataSet.append(SharedPipeline(&it->second), pipelineState);
	creationDataSet.createPipelines(*_pipelineLibrary);
	return sharedPipeline;
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
		it->second._mapIterator = it;
		it->second._primitiveTopology = shaderState.primitiveTopology;
	}
	return it->second.getOrCreatePipeline(pipelineState);
}


void PipelineLibrary::setProjectionViewportAndScissor(const std::vector<glm::mat4x4>& projectionMatrixList,
	const std::vector<vk::Viewport>& viewportList, const std::vector<vk::Rect2D>& scissorList)
{
	// update specialization constants based on projection matrices
	_specializationData.resize(projectionMatrixList.size());
	for(size_t i=0,c=projectionMatrixList.size(); i<c; i++)
	{
		const auto& projectionMatrix = projectionMatrixList[i];
		_specializationData[i] =
			array<float,6>{
				projectionMatrix[2][0], projectionMatrix[2][1], projectionMatrix[2][3],
				projectionMatrix[3][0], projectionMatrix[3][1], projectionMatrix[3][3],
			};
	}

	// update viewports and scissors
	_viewportList = viewportList;
	_scissorList = scissorList;

	// CreationDataSet - used for storing all pipeline creation data
	// so we create pipelines in batches and not one by one.
	// This might allow Vulkan driver for speeding up creation.
	CreationDataSet creationDataSet(*this);

	// create list of pipelines to be recompiled
	for(auto familyIt=_pipelineFamilyMap.begin(); familyIt!=_pipelineFamilyMap.end(); familyIt++) {
		const ShaderState& shaderState = familyIt->first;
		PipelineFamily& f = familyIt->second;
		for(auto pipelineIt=f._pipelineMap.begin(); pipelineIt!=f._pipelineMap.end(); pipelineIt++) {
			const PipelineState& pipelineState = pipelineIt->first;
			if(pipelineState.viewportAndScissorHandling == PipelineState::ViewportAndScissorHandling::SetFunction)
			{
				// update viewport and scissor
				const_cast<vk::Viewport&>(pipelineState.viewport) =
					viewportList.at(pipelineState.viewportIndex);
				const_cast<vk::Rect2D&>(pipelineState.scissor) =
					scissorList.at(pipelineState.scissorIndex);

				// append pipeline into the set for recompilation
				creationDataSet.append(SharedPipeline(&pipelineIt->second), pipelineState);
			}
			else if(shaderState.projectionHandling == ShaderState::ProjectionHandling::PerspectivePushAndSpecializationConstants)
				// append pipeline into the set for recompilation
				creationDataSet.append(SharedPipeline(&pipelineIt->second), pipelineState);
		}
	}

	// create pipelines
	creationDataSet.createPipelines(*this);
}


PipelineLibrary::CreationDataSet::CreationDataSet(const PipelineLibrary& pipelineLibrary)
	: specializationList(pipelineLibrary._specializationData.size())
	, viewportList(pipelineLibrary._viewportList)
	, scissorList(pipelineLibrary._scissorList)
{
	for(size_t i=0,c=pipelineLibrary._specializationData.size(); i<c; i++) {
		auto& s = specializationList[i];
		get<0>(s) = pipelineLibrary._specializationData[i];
		get<1>(s) =
			vk::SpecializationInfo(
				uint32_t(specializationMap.size()),  // mapEntryCount
				specializationMap.data(),  // pMapEntries
				6 * sizeof(float),  // dataSize
				get<0>(s).data()  // pData
			);

	}
}


void PipelineLibrary::CreationDataBatch::append(SharedPipeline&& sharedPipeline, const PipelineState& pipelineState)
{
	assert(numSharedPipelines < sharedPipelineList.size() && "CreationDataBatch::append(): CreationDataBatch is full. Cannot append more pipelines.");
	assert(sharedPipeline.cadrPipeline() != nullptr && "SharedPipeline object must not be empty.");

	// get info from sharedPipeline before we move it
	const PipelineFamily& pipelineFamily = *sharedPipeline.pipelineFamily();
	vk::PipelineLayout pipelineLayout = pipelineFamily._pipelineLibrary->pipelineLayout();
	const ShaderState& shaderState = pipelineFamily.shaderState();

	// move sharedPipeline
	sharedPipelineList[numSharedPipelines] = sharedPipeline;
	numSharedPipelines++;

	// flags
	auto& createInfo = createInfoList[numCreateInfos];
	createInfo.flags = vk::PipelineCreateFlags();
	numCreateInfos++;

	// specializationInfo
	vk::SpecializationInfo* specializationInfo =
		(shaderState.projectionHandling == ShaderState::ProjectionHandling::PerspectivePushAndSpecializationConstants)
			? &get<1>(creationDataSet->specializationList.at(pipelineState.projectionIndex))
			: nullptr;

	// stageCount and pStages
	auto& shaderStages = shaderStageList[numShaderStages];
	numShaderStages += 3;
	shaderStages[0] =
		vk::PipelineShaderStageCreateInfo{
			vk::PipelineShaderStageCreateFlags(),  // flags
			vk::ShaderStageFlagBits::eVertex,  // stage
			pipelineFamily._vertexShader,  // module
			"main",  // pName
			specializationInfo,  // pSpecializationInfo
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
				specializationInfo,  // pSpecializationInfo
			};
		createInfo.stageCount = 3;
	}
	else
		createInfo.stageCount = 2;
	createInfo.pStages = &shaderStages;

	// pVertexInputState
	createInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;

	// pInputAssemblyState
	for(size_t i=0; i<numInputAssemblyStates; i++)
		if(inputAssemblyStateList[i].topology == pipelineFamily._primitiveTopology) {
			createInfo.pInputAssemblyState = &inputAssemblyStateList[i];
			goto foundInputAssemblyState;
		}
	inputAssemblyStateList[numInputAssemblyStates] =
		vk::PipelineInputAssemblyStateCreateInfo(
			vk::PipelineInputAssemblyStateCreateFlags(),  // flags
			pipelineFamily._primitiveTopology,  // topology
			VK_FALSE  // primitiveRestartEnable
		);
	createInfo.pInputAssemblyState = &inputAssemblyStateList[numInputAssemblyStates];
	numInputAssemblyStates++;
	foundInputAssemblyState:;

	// pTessellationState
	createInfo.pTessellationState = nullptr;

	// pViewportState
	for(size_t i=0; i<numViewportStates; i++)
		if(get<1>(viewportStateList[i]) == pipelineState.viewport &&
		   get<2>(viewportStateList[i]) == pipelineState.scissor)
		{
			createInfo.pViewportState = &get<0>(viewportStateList[i]);
			goto foundViewportState;
		}
	get<0>(viewportStateList[numViewportStates]) =
		vk::PipelineViewportStateCreateInfo(
			vk::PipelineViewportStateCreateFlags(),  // flags
			1,  // viewportCount
			&get<1>(viewportStateList[numViewportStates]),  // pViewports
			1,  // scissorCount
			&get<2>(viewportStateList[numViewportStates])  // pScissors
		);
	get<1>(viewportStateList[numViewportStates]) = pipelineState.viewport;
	get<2>(viewportStateList[numViewportStates]) = pipelineState.scissor;
	createInfo.pViewportState = &get<0>(viewportStateList[numViewportStates]);
	numViewportStates++;
	foundViewportState:;

	// pRasterizationState
	for(size_t i=0; i<numRasterizationStates; i++) {
		const auto& rasterizationState = rasterizationStateList[i];
		if(rasterizationState.cullMode == pipelineState.cullMode &&
		   rasterizationState.frontFace == pipelineState.frontFace &&
		   (rasterizationState.depthBiasEnable!=0) == pipelineState.depthBiasEnable &&
		   rasterizationState.depthBiasConstantFactor == pipelineState.depthBiasConstantFactor &&
		   rasterizationState.depthBiasClamp == pipelineState.depthBiasClamp &&
		   rasterizationState.depthBiasSlopeFactor == pipelineState.depthBiasSlopeFactor &&
		   rasterizationState.lineWidth == pipelineState.lineWidth)
		{
			createInfo.pRasterizationState = &rasterizationStateList[i];
			goto foundRasterizationState;
		}
	}
	rasterizationStateList[numRasterizationStates] =
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
		};
	createInfo.pRasterizationState = &rasterizationStateList[numRasterizationStates];
	numRasterizationStates++;
	foundRasterizationState:;

	// pMultisampleState
	for(size_t i=0; i<numMultisampleStates; i++) {
		const auto& multisampleState = multisampleStateList[i];
		if(multisampleState.rasterizationSamples == pipelineState.rasterizationSamples &&
		   (multisampleState.sampleShadingEnable!=0) == pipelineState.sampleShadingEnable &&
		   multisampleState.minSampleShading == pipelineState.minSampleShading)
		{
			createInfo.pMultisampleState = &multisampleStateList[i];
			goto foundMultisampleState;
		}
	}
	multisampleStateList[numMultisampleStates] =
		vk::PipelineMultisampleStateCreateInfo{
			vk::PipelineMultisampleStateCreateFlags(),  // flags
			pipelineState.rasterizationSamples,  // rasterizationSamples
			pipelineState.sampleShadingEnable,  // sampleShadingEnable
			pipelineState.minSampleShading,  // minSampleShading
			nullptr,   // pSampleMask
			VK_FALSE,  // alphaToCoverageEnable
			VK_FALSE   // alphaToOneEnable
		};
	createInfo.pMultisampleState = &multisampleStateList[numMultisampleStates];
	numMultisampleStates++;
	foundMultisampleState:;

	// pDepthStencilState
	for(size_t i=0; i<numDepthStencilStates; i++) {
		const auto& depthStencilState = depthStencilStateList[i];
		if((depthStencilState.depthTestEnable!=0) == pipelineState.depthTestEnable &&
		   (depthStencilState.depthWriteEnable!=0) == pipelineState.depthWriteEnable)
		{
			createInfo.pDepthStencilState = &depthStencilStateList[i];
			goto foundDepthStencilState;
		}
	}
	depthStencilStateList[numDepthStencilStates] =
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
		};
	createInfo.pDepthStencilState = &depthStencilStateList[numDepthStencilStates];
	numDepthStencilStates++;
	foundDepthStencilState:;

	// colorBlendAttachmentState
	vk::PipelineColorBlendAttachmentState* colorBlendAttachmentsPtr = nullptr;
	auto isBlendAttachmentStateEqual =
		[](const vk::PipelineColorBlendAttachmentState& s1,
		   const PipelineState::BlendAttachmentState& s2) -> bool
		{
			if((s1.blendEnable!=0) != s2.blendEnable || s1.colorWriteMask != s2.colorWriteMask)
				return false;
			if(s2.blendEnable == false)  // blendEnable is the same on s1 and s2; if both are false, no need to compare blend settings
				return true;
			return s1.srcColorBlendFactor != s2.srcColorBlendFactor ||
			       s1.dstColorBlendFactor != s2.dstColorBlendFactor ||
			       s1.colorBlendOp        != s2.colorBlendOp ||
			       s1.srcAlphaBlendFactor != s2.srcAlphaBlendFactor ||
			       s1.dstAlphaBlendFactor != s2.dstAlphaBlendFactor ||
			       s1.alphaBlendOp        != s2.alphaBlendOp;
		};
	for(size_t i=0,c=numColorBlendAttachmentStates; i<c; i+=numAttachmentsPerPipeline) {
		for(size_t j=0,d=pipelineState.blendState.size(); j<d; j++)
			if(!isBlendAttachmentStateEqual(colorBlendAttachmentStateList[i+j], pipelineState.blendState[j]))
				goto colorBlendAttachmentsDiffer;
		colorBlendAttachmentsPtr = &colorBlendAttachmentStateList[i];
		goto colorBlendAttachmentsFound;
		colorBlendAttachmentsDiffer:;
	}
	colorBlendAttachmentsPtr = &colorBlendAttachmentStateList[numColorBlendAttachmentStates];
	for(size_t i=0,c=pipelineState.blendState.size(); i<c; i++) {
		const auto& src = pipelineState.blendState[i];
		colorBlendAttachmentStateList[numColorBlendAttachmentStates+i] =
			vk::PipelineColorBlendAttachmentState(
				src.blendEnable,
				src.srcColorBlendFactor,
				src.dstColorBlendFactor,
				src.colorBlendOp,
				src.srcAlphaBlendFactor,
				src.dstAlphaBlendFactor,
				src.alphaBlendOp,
				src.colorWriteMask
			);
	}
	numColorBlendAttachmentStates += numAttachmentsPerPipeline;
	colorBlendAttachmentsFound:;

	// pColorBlendState
	for(size_t i=0; i<numColorBlendStates; i++) {
		const auto& colorBlendState = colorBlendStateList[i];
		if(colorBlendState.attachmentCount != pipelineState.blendState.size())
			continue;
		if(colorBlendState.pAttachments != colorBlendAttachmentsPtr)
			continue;
		createInfo.pColorBlendState = &colorBlendState;
		goto foundColorBlendState;
	}
	colorBlendStateList[numColorBlendStates] =
		vk::PipelineColorBlendStateCreateInfo(
			vk::PipelineColorBlendStateCreateFlags(),  // flags
			VK_FALSE,  // logicOpEnable
			vk::LogicOp::eClear,  // logicOp
			uint32_t(pipelineState.blendState.size()),  // attachmentCount
			colorBlendAttachmentsPtr,  // pAttachments
			array<float,4>{0.f,0.f,0.f,0.f}  // blendConstants
		);
	createInfo.pColorBlendState = &colorBlendStateList[numColorBlendStates];
	numColorBlendStates++;
	foundColorBlendState:;

	// pDynamicState
	createInfo.pDynamicState = nullptr;

	// remaining createInfo members
	createInfo.layout = pipelineLayout;
	createInfo.renderPass = pipelineState.renderPass;
	createInfo.subpass = pipelineState.subpass;
	createInfo.basePipelineHandle = nullptr;
	createInfo.basePipelineIndex = -1;
}


[[nodiscard]] array<vk::Pipeline,PipelineLibrary::CreationDataBatch::numPipelines>
	PipelineLibrary::CreationDataBatch::createPipelines(
		CadR::VulkanDevice& device, vk::PipelineCache pipelineCache)
{
	// create pipelines
	array<vk::Pipeline,numPipelines> pipelines;
	VkResult r =
		device.vkCreateGraphicsPipelines(
			device.handle(),
			pipelineCache,
			numCreateInfos,
			reinterpret_cast<VkGraphicsPipelineCreateInfo*>(createInfoList.data()),
			nullptr,
			reinterpret_cast<VkPipeline*>(pipelines.data())
		);
	if(r != VK_SUCCESS) {
		for(vk::Pipeline p : pipelines)
			device.destroy(p);
	#if VK_HEADER_VERSION < 256  // throwResultException moved to detail namespace on 2023-06-28 and the change went public in 1.3.256
		vk::throwResultException(vk::Result(r), "vk::Device::createGraphicsPipelines");
	#else
		vk::detail::throwResultException(vk::Result(r), "vk::Device::createGraphicsPipelines");
	#endif
	}
	return pipelines;
}


void PipelineLibrary::CreationDataSet::createPipelines(const PipelineLibrary& pipelineLibrary)
{
	array<vk::Pipeline,CreationDataBatch::numPipelines> pipelines;
	while(!batchList.empty())
	{
		CreationDataBatch& batch = batchList.front();

		// create pipelines
		// note: do not throw in the following code until pipelines are safely replaced through
		// SharedPipeline objects; otherwise pipeline handles will be leaked
		pipelines = batch.createPipelines(*pipelineLibrary._device, pipelineLibrary._pipelineCache);

		// update pipelines inside SharedPipeline object
		for(size_t i=0, c=batch.numSharedPipelines; i<c; i++)
			batch.sharedPipelineList[i].replacePipelineHandle(pipelines[i], *pipelineLibrary._device);

		// release CreationDataBatch
		batchList.pop_front();
	}
}
