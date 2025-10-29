#include "PipelineLibrary.h"
#include <CadR/VulkanDevice.h>
#include <array>
#include <cstdint>
#include <tuple>

using namespace std;

// shader code in SPIR-V binary
static const uint32_t baseColorOverallMaterialVertexShaderSpirv[]={
#include "shaders/baseColor-overallMaterial.vert.spv"
};
static const uint32_t baseColorPointOverallMaterialVertexShaderSpirv[]={
#include "shaders/baseColorPoint-overallMaterial.vert.spv"
};
static const uint32_t baseColorOverallMaterialFragmentShaderSpirv[]={
#include "shaders/baseColor-overallMaterial.frag.spv"
};
static const uint32_t baseColorOverallMaterialTexturingVertexShaderSpirv[]={
#include "shaders/baseColor-overallMaterial-texturing.vert.spv"
};
static const uint32_t baseColorPointOverallMaterialTexturingVertexShaderSpirv[]={
#include "shaders/baseColorPoint-overallMaterial-texturing.vert.spv"
};
static const uint32_t baseColorOverallMaterialTexturingFragmentShaderSpirv[]={
#include "shaders/baseColor-overallMaterial-texturing.frag.spv"
};
static const uint32_t baseColorPerVertexColorVertexShaderSpirv[]={
#include "shaders/baseColor-perVertexColor.vert.spv"
};
static const uint32_t baseColorPointPerVertexColorVertexShaderSpirv[]={
#include "shaders/baseColorPoint-perVertexColor.vert.spv"
};
static const uint32_t baseColorPerVertexColorFragmentShaderSpirv[]={
#include "shaders/baseColor-perVertexColor.frag.spv"
};
static const uint32_t baseColorPerVertexColorTexturingVertexShaderSpirv[]={
#include "shaders/baseColor-perVertexColor-texturing.vert.spv"
};
static const uint32_t baseColorPointPerVertexColorTexturingVertexShaderSpirv[]={
#include "shaders/baseColorPoint-perVertexColor-texturing.vert.spv"
};
static const uint32_t baseColorPerVertexColorTexturingFragmentShaderSpirv[]={
#include "shaders/baseColor-perVertexColor-texturing.frag.spv"
};
static const uint32_t phongOverallMaterialVertexShaderSpirv[]={
#include "shaders/phong-overallMaterial.vert.spv"
};
static const uint32_t phongPointOverallMaterialVertexShaderSpirv[]={
#include "shaders/phongPoint-overallMaterial.vert.spv"
};
static const uint32_t phongOverallMaterialFragmentShaderSpirv[]={
#include "shaders/phong-overallMaterial.frag.spv"
};
static const uint32_t phongOverallMaterialTexturingVertexShaderSpirv[]={
#include "shaders/phong-overallMaterial-texturing.vert.spv"
};
static const uint32_t phongPointOverallMaterialTexturingVertexShaderSpirv[]={
#include "shaders/phongPoint-overallMaterial-texturing.vert.spv"
};
static const uint32_t phongOverallMaterialTexturingFragmentShaderSpirv[]={
#include "shaders/phong-overallMaterial-texturing.frag.spv"
};
static const uint32_t phongPerVertexColorVertexShaderSpirv[]={
#include "shaders/phong-perVertexColor.vert.spv"
};
static const uint32_t phongPointPerVertexColorVertexShaderSpirv[]={
#include "shaders/phongPoint-perVertexColor.vert.spv"
};
static const uint32_t phongPerVertexColorFragmentShaderSpirv[]={
#include "shaders/phong-perVertexColor.frag.spv"
};
static const uint32_t phongPerVertexColorTexturingVertexShaderSpirv[]={
#include "shaders/phong-perVertexColor-texturing.vert.spv"
};
static const uint32_t phongPointPerVertexColorTexturingVertexShaderSpirv[]={
#include "shaders/phongPoint-perVertexColor-texturing.vert.spv"
};
static const uint32_t phongPerVertexColorTexturingFragmentShaderSpirv[]={
#include "shaders/phong-perVertexColor-texturing.frag.spv"
};

static constexpr array vertexShaderSpirvList = {
	tuple{ baseColorOverallMaterialVertexShaderSpirv,          sizeof(baseColorOverallMaterialVertexShaderSpirv) },
	tuple{ baseColorPerVertexColorVertexShaderSpirv,           sizeof(baseColorPerVertexColorVertexShaderSpirv) },
	tuple{ baseColorOverallMaterialTexturingVertexShaderSpirv, sizeof(baseColorOverallMaterialTexturingVertexShaderSpirv) },
	tuple{ baseColorPerVertexColorTexturingVertexShaderSpirv,  sizeof(baseColorPerVertexColorTexturingVertexShaderSpirv) },
	tuple{ phongOverallMaterialVertexShaderSpirv,          sizeof(phongOverallMaterialVertexShaderSpirv) },
	tuple{ phongPerVertexColorVertexShaderSpirv,           sizeof(phongPerVertexColorVertexShaderSpirv) },
	tuple{ phongOverallMaterialTexturingVertexShaderSpirv, sizeof(phongOverallMaterialTexturingVertexShaderSpirv) },
	tuple{ phongPerVertexColorTexturingVertexShaderSpirv,  sizeof(phongPerVertexColorTexturingVertexShaderSpirv) },
};
static constexpr array fragmentShaderSpirvList = {
	tuple{ baseColorOverallMaterialFragmentShaderSpirv,          sizeof(baseColorOverallMaterialFragmentShaderSpirv) },
	tuple{ baseColorPerVertexColorFragmentShaderSpirv,           sizeof(baseColorPerVertexColorFragmentShaderSpirv) },
	tuple{ baseColorOverallMaterialTexturingFragmentShaderSpirv, sizeof(baseColorOverallMaterialTexturingFragmentShaderSpirv) },
	tuple{ baseColorPerVertexColorTexturingFragmentShaderSpirv,  sizeof(baseColorPerVertexColorTexturingFragmentShaderSpirv) },
	tuple{ phongOverallMaterialFragmentShaderSpirv,          sizeof(phongOverallMaterialFragmentShaderSpirv) },
	tuple{ phongPerVertexColorFragmentShaderSpirv,           sizeof(phongPerVertexColorFragmentShaderSpirv) },
	tuple{ phongOverallMaterialTexturingFragmentShaderSpirv, sizeof(phongOverallMaterialTexturingFragmentShaderSpirv) },
	tuple{ phongPerVertexColorTexturingFragmentShaderSpirv,  sizeof(phongPerVertexColorTexturingFragmentShaderSpirv) },
};
static constexpr array pointVertexShaderSpirvList = {
	tuple{ baseColorPointOverallMaterialVertexShaderSpirv,          sizeof(baseColorPointOverallMaterialVertexShaderSpirv) },
	tuple{ baseColorPointPerVertexColorVertexShaderSpirv,           sizeof(baseColorPointPerVertexColorVertexShaderSpirv) },
	tuple{ baseColorPointOverallMaterialTexturingVertexShaderSpirv, sizeof(baseColorPointOverallMaterialTexturingVertexShaderSpirv) },
	tuple{ baseColorPointPerVertexColorTexturingVertexShaderSpirv,  sizeof(baseColorPointPerVertexColorTexturingVertexShaderSpirv) },
	tuple{ phongPointOverallMaterialVertexShaderSpirv,          sizeof(phongPointOverallMaterialVertexShaderSpirv) },
	tuple{ phongPointPerVertexColorVertexShaderSpirv,           sizeof(phongPointPerVertexColorVertexShaderSpirv) },
	tuple{ phongPointOverallMaterialTexturingVertexShaderSpirv, sizeof(phongPointOverallMaterialTexturingVertexShaderSpirv) },
	tuple{ phongPointPerVertexColorTexturingVertexShaderSpirv,  sizeof(phongPointPerVertexColorTexturingVertexShaderSpirv) },
};



void PipelineLibrary::destroy() noexcept
{
	if(_device == nullptr)
		return;

	for(vk::Pipeline& p : _pipelines) {
		_device->destroy(p);
		p = nullptr;
	}
	_device->destroy(_pipelineLayout);
	_pipelineLayout = nullptr;
	_device->destroy(_texturePipelineLayout);
	_texturePipelineLayout = nullptr;
	_device->destroy(_textureDescriptorSetLayout);
	_textureDescriptorSetLayout = nullptr;
	for(vk::ShaderModule& sm : _vertexShaders) {
		_device->destroy(sm);
		sm = nullptr;
	}
	for(vk::ShaderModule& sm : _fragmentShaders) {
		_device->destroy(sm);
		sm = nullptr;
	}
	for(vk::ShaderModule& sm : _pointVertexShaders) {
		_device->destroy(sm);
		sm = nullptr;
	}
	_device = nullptr;
}


// pipeline vertex input
static constexpr const vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo(
	vk::PipelineVertexInputStateCreateFlags(),  // flags
	0,  // vertexBindingDescriptionCount
	nullptr,  // pVertexBindingDescriptions
	0,  // vertexAttributeDescriptionCount
	nullptr  // pVertexAttributeDescriptions
);


void PipelineLibrary::create(CadR::VulkanDevice& device)
{
	if(_device != &device)
	{
		destroy();
		
		_device = &device;

		for(size_t i=0; i<vertexShaderSpirvList.size(); i++)
			_vertexShaders[i] =
				_device->createShaderModule(
					vk::ShaderModuleCreateInfo(
						vk::ShaderModuleCreateFlags(),  // flags
						get<1>(vertexShaderSpirvList[i]),  // codeSize
						get<0>(vertexShaderSpirvList[i])   // pCode
					)
				);
		for(size_t i=0; i<fragmentShaderSpirvList.size(); i++)
			_fragmentShaders[i] =
				_device->createShaderModule(
					vk::ShaderModuleCreateInfo(
						vk::ShaderModuleCreateFlags(),    // flags
						get<1>(fragmentShaderSpirvList[i]),  // codeSize
						get<0>(fragmentShaderSpirvList[i])   // pCode
					)
				);
		for(size_t i=0; i<pointVertexShaderSpirvList.size(); i++)
			_pointVertexShaders[i] =
				_device->createShaderModule(
					vk::ShaderModuleCreateInfo(
						vk::ShaderModuleCreateFlags(),  // flags
						get<1>(pointVertexShaderSpirvList[i]),  // codeSize
						get<0>(pointVertexShaderSpirvList[i])   // pCode
					)
				);
		_textureDescriptorSetLayout =
			_device->createDescriptorSetLayout(
				vk::DescriptorSetLayoutCreateInfo(
					vk::DescriptorSetLayoutCreateFlags(),  // flags
					1,  // bindingCount
					array<vk::DescriptorSetLayoutBinding,1>{
						vk::DescriptorSetLayoutBinding{
							0,  // binding
							vk::DescriptorType::eCombinedImageSampler,  // descriptorType
							1, // descriptorCount
							vk::ShaderStageFlagBits::eFragment,  // stageFlags
							nullptr  // pImmutableSamplers
						}
					}.data()
				)
			);
		_pipelineLayout =
			_device->createPipelineLayout(
				vk::PipelineLayoutCreateInfo(
					vk::PipelineLayoutCreateFlags(),  // flags
					0,  // setLayoutCount
					nullptr,  // pSetLayouts
					2,  // pushConstantRangeCount
					array{
						vk::PushConstantRange{  // pPushConstantRanges
							vk::ShaderStageFlagBits::eVertex,  // stageFlags
							0,  // offset
							2*sizeof(uint64_t)  // size
						},
						vk::PushConstantRange{  // pPushConstantRanges
							vk::ShaderStageFlagBits::eFragment,  // stageFlags
							8,  // offset
							sizeof(uint64_t) + sizeof(uint32_t)  // size
						},
					}.data()
				)
			);
		_texturePipelineLayout =
			_device->createPipelineLayout(
				vk::PipelineLayoutCreateInfo(
					vk::PipelineLayoutCreateFlags(),  // flags
					1,  // setLayoutCount
					&_textureDescriptorSetLayout,  // pSetLayouts
					2,  // pushConstantRangeCount
					array{
						vk::PushConstantRange{  // pPushConstantRanges
							vk::ShaderStageFlagBits::eVertex,  // stageFlags
							0,  // offset
							2*sizeof(uint64_t)  // size
						},
						vk::PushConstantRange{  // pPushConstantRanges
							vk::ShaderStageFlagBits::eFragment,  // stageFlags
							8,  // offset
							sizeof(uint64_t) + sizeof(uint32_t)  // size
						},
					}.data()
				)
			);
	}
}


void PipelineLibrary::create(CadR::VulkanDevice& device, vk::Extent2D surfaceExtent,
                             const vk::SpecializationInfo& specializationInfo, vk::RenderPass renderPass,
                             vk::PipelineCache pipelineCache)
{
	// init shaders
	// and handle device change
	create(device);

	// destroy previous pipelines
	for(vk::Pipeline& p : _pipelines) {
		_device->destroy(p);
		p = nullptr;
	}

	array<vk::PipelineShaderStageCreateInfo, 2> shaderStages{
		vk::PipelineShaderStageCreateInfo{
			vk::PipelineShaderStageCreateFlags(),  // flags
			vk::ShaderStageFlagBits::eVertex,  // stage
			nullptr,  // module - will be set later
			"main",  // pName
			&specializationInfo,  // pSpecializationInfo
		},
		vk::PipelineShaderStageCreateInfo{
			vk::PipelineShaderStageCreateFlags(),  // flags
			vk::ShaderStageFlagBits::eFragment,  // stage
			nullptr,  // module - will be set later
			"main",  // pName
			&specializationInfo,  // pSpecializationInfo
		},
	};
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState{
		vk::PipelineInputAssemblyStateCreateFlags(),  // flags
		vk::PrimitiveTopology::eTriangleList,  // topology - will be set later
		VK_FALSE  // primitiveRestartEnable
	};
	vk::Viewport viewport(0.f, 0.f, float(surfaceExtent.width), float(surfaceExtent.height), 0.f, 1.f);
	vk::Rect2D scissor(vk::Offset2D(0, 0), surfaceExtent);
	vk::PipelineViewportStateCreateInfo viewportState{
		vk::PipelineViewportStateCreateFlags(),  // flags
		1,  // viewportCount
		&viewport,  // pViewports
		1,  // scissorCount
		&scissor  // pScissors
	};
	vk::PipelineRasterizationStateCreateInfo rasterizationState{
		vk::PipelineRasterizationStateCreateFlags(),  // flags
		VK_FALSE,  // depthClampEnable
		VK_FALSE,  // rasterizerDiscardEnable
		vk::PolygonMode::eFill,  // polygonMode
		vk::CullModeFlagBits::eNone,  // cullMode - will be set later
		vk::FrontFace::eCounterClockwise,  // frontFace - will be set later
		VK_FALSE,  // depthBiasEnable
		0.f,  // depthBiasConstantFactor
		0.f,  // depthBiasClamp
		0.f,  // depthBiasSlopeFactor
		1.f   // lineWidth
	};
	vk::PipelineMultisampleStateCreateInfo multisampleState{
		vk::PipelineMultisampleStateCreateFlags(),  // flags
		vk::SampleCountFlagBits::e1,  // rasterizationSamples
		VK_FALSE,  // sampleShadingEnable
		0.f,       // minSampleShading
		nullptr,   // pSampleMask
		VK_FALSE,  // alphaToCoverageEnable
		VK_FALSE   // alphaToOneEnable
	};
	vk::PipelineDepthStencilStateCreateInfo depthStencilState{
		vk::PipelineDepthStencilStateCreateFlags(),  // flags
		VK_TRUE,  // depthTestEnable
		VK_TRUE,  // depthWriteEnable
		vk::CompareOp::eLess,  // depthCompareOp
		VK_FALSE,  // depthBoundsTestEnable
		VK_FALSE,  // stencilTestEnable
		vk::StencilOpState(),  // front
		vk::StencilOpState(),  // back
		0.f,  // minDepthBounds
		0.f   // maxDepthBounds
	};
	array colorBlendAttachmentStates{
		vk::PipelineColorBlendAttachmentState{
			VK_FALSE,  // blendEnable
			vk::BlendFactor::eZero,  // srcColorBlendFactor
			vk::BlendFactor::eZero,  // dstColorBlendFactor
			vk::BlendOp::eAdd,       // colorBlendOp
			vk::BlendFactor::eZero,  // srcAlphaBlendFactor
			vk::BlendFactor::eZero,  // dstAlphaBlendFactor
			vk::BlendOp::eAdd,       // alphaBlendOp
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA  // colorWriteMask
		},
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
		renderPass,  // renderPass
		0,  // subpass
		nullptr,  // basePipelineHandle
		-1 // basePipelineIndex
	);

	// create new triangle pipelines
	inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
	for(size_t i=0; i<_numTrianglePipelines; i++) {
		shaderStages[0].module = _vertexShaders[i&0x7];
		shaderStages[1].module = _fragmentShaders[i&0x7];
		rasterizationState.cullMode =
			(i&0x08)==0 ? vk::CullModeFlagBits::eNone :  // eNone - nothing is discarded,
			              vk::CullModeFlagBits::eBack;  // eBack - back-facing triangles are discarded, eFront - front-facing triangles are discarded
		rasterizationState.frontFace =
			(i&0x10) ? vk::FrontFace::eClockwise : vk::FrontFace::eCounterClockwise;
		createInfo.layout =
			(i&0x02) ? _texturePipelineLayout : _pipelineLayout;
		_pipelines[i] =
			_device->createGraphicsPipeline(
				pipelineCache,  // pipelineCache
				createInfo  // createInfo
			);
	}

	// create new line pipelines
	inputAssemblyState.topology = vk::PrimitiveTopology::eLineList;
	rasterizationState.cullMode = vk::CullModeFlagBits::eNone;  // lines do not perform visibility culling
	rasterizationState.frontFace = vk::FrontFace::eCounterClockwise;  // frontFace does not matter for lines
	static_assert((_numTrianglePipelines&0x7) == 0, "Number of triangle pipelines must be multiple of 8.");
	for(size_t i=_numTrianglePipelines,e=i+_numLinePipelines; i<e; i++) {
		shaderStages[0].module = _vertexShaders[i&0x7];
		shaderStages[1].module = _fragmentShaders[i&0x7];
		_pipelines[i] =
			_device->createGraphicsPipeline(
				pipelineCache,  // pipelineCache
				createInfo  // createInfo
			);
	}

	// create new point pipelines
	inputAssemblyState.topology = vk::PrimitiveTopology::ePointList;
	static_assert((_numLinePipelines&0x7) == 0, "Number of line pipelines must be multiple of 8.");
	for(size_t i=_numTrianglePipelines+_numLinePipelines,e=i+_numPointPipelines; i<e; i++) {
		shaderStages[0].module = _pointVertexShaders[i&0x7];
		shaderStages[1].module = _fragmentShaders[i&0x7];
		_pipelines[i] =
			_device->createGraphicsPipeline(
				pipelineCache,  // pipelineCache
				createInfo  // createInfo
			);
	}
}
