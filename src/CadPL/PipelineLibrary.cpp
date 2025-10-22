#include <CadPL/PipelineLibrary.h>

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


void PipelineFamily::destroyPipeline(void* pipelineOwner) noexcept
{
	PipelineOwner* po = reinterpret_cast<PipelineOwner*>(pipelineOwner); 
	PipelineFamily* pf = po->pipelineFamily;
	pf->_device->destroy(po->pipeline);
	pf->_pipelineMap.erase(po->eraseIt);

	if(pf->_pipelineMap.empty())
		pf->_pipelineLibrary->_pipelineFamilyMap.erase(pf->_eraseIt);
}


SharedPipeline PipelineFamily::getOrCreatePipeline(const PipelineState& pipelineState)
{
	auto [it, newRecord] = _pipelineMap.try_emplace(pipelineState);
	if(newRecord) {
		try {
			it->second.pipeline = createPipeline(pipelineState);
		} catch(...) {
			_pipelineMap.erase(it);
			throw;
		}
		it->second.referenceCounter = 0;
		it->second.pipelineFamily = this;
		it->second.eraseIt = it;
	}
	return SharedPipeline(&it->second);
}


SharedPipeline PipelineLibrary::getOrCreatePipeline(const ShaderState& shaderState, const PipelineState& pipelineState)
{
	auto [it, newRecord] = _pipelineFamilyMap.try_emplace(shaderState, *this);
	if(newRecord) {
		try {
			it->second._vertexShader = _shaderLibrary->getVertexShader(shaderState);
			it->second._geometryShader = _shaderLibrary->getGeometryShader(shaderState);
			it->second._fragmentShader = _shaderLibrary->getFragmentShader(shaderState);
		} catch(...) {
			_pipelineFamilyMap.erase(it);
			throw;
		}
		it->second.setEraseIt(it);
	}
	return it->second.getOrCreatePipeline(pipelineState);
}


vk::Pipeline PipelineFamily::createPipeline(const PipelineState& pipelineState)
{
	vk::SpecializationInfo specializationInfo{
		uint32_t(pipelineState.specializationMap.size()),  // mapEntryCount
		pipelineState.specializationMap.data(),  // pMapEntries
		pipelineState.specializationData.size(),  // dataSize
		pipelineState.specializationData.data(),  // pData
	};
	array<vk::PipelineShaderStageCreateInfo, 3> shaderStages{
		vk::PipelineShaderStageCreateInfo{
			vk::PipelineShaderStageCreateFlags(),  // flags
			vk::ShaderStageFlagBits::eVertex,  // stage
			_vertexShader,  // module
			"main",  // pName
			&specializationInfo,  // pSpecializationInfo
		},
		vk::PipelineShaderStageCreateInfo{
			vk::PipelineShaderStageCreateFlags(),  // flags
			vk::ShaderStageFlagBits::eFragment,  // stage
			_fragmentShader,  // module
			"main",  // pName
			&specializationInfo,  // pSpecializationInfo
		},
		vk::PipelineShaderStageCreateInfo{
			vk::PipelineShaderStageCreateFlags(),  // flags
			vk::ShaderStageFlagBits::eGeometry,  // stage
			_geometryShader,  // module
			"main",  // pName
			&specializationInfo,  // pSpecializationInfo
		},
	};
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState{
		vk::PipelineInputAssemblyStateCreateFlags(),  // flags
		pipelineState.primitiveTopology,  // topology
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
		pipelineState.depthBiasConstant,  // depthBiasConstantFactor
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
		nullptr,  // layout - will be set later
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
