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
static const uint32_t baseColorOverallMaterialFragmentShaderSpirv[]={
#include "shaders/baseColor-overallMaterial.frag.spv"
};
static const uint32_t baseColorOverallMaterialTexturingVertexShaderSpirv[]={
#include "shaders/baseColor-overallMaterial-texturing.vert.spv"
};
static const uint32_t baseColorOverallMaterialTexturingFragmentShaderSpirv[]={
#include "shaders/baseColor-overallMaterial-texturing.frag.spv"
};
static const uint32_t baseColorPerVertexColorVertexShaderSpirv[]={
#include "shaders/baseColor-perVertexColor.vert.spv"
};
static const uint32_t baseColorPerVertexColorFragmentShaderSpirv[]={
#include "shaders/baseColor-perVertexColor.frag.spv"
};
static const uint32_t baseColorPerVertexColorTexturingVertexShaderSpirv[]={
#include "shaders/baseColor-perVertexColor-texturing.vert.spv"
};
static const uint32_t baseColorPerVertexColorTexturingFragmentShaderSpirv[]={
#include "shaders/baseColor-perVertexColor-texturing.frag.spv"
};
static const uint32_t phongOverallMaterialVertexShaderSpirv[]={
#include "shaders/phong-overallMaterial.vert.spv"
};
static const uint32_t phongOverallMaterialFragmentShaderSpirv[]={
#include "shaders/phong-overallMaterial.frag.spv"
};
static const uint32_t phongOverallMaterialTexturingVertexShaderSpirv[]={
#include "shaders/phong-overallMaterial-texturing.vert.spv"
};
static const uint32_t phongOverallMaterialTexturingFragmentShaderSpirv[]={
#include "shaders/phong-overallMaterial-texturing.frag.spv"
};
static const uint32_t phongPerVertexColorVertexShaderSpirv[]={
#include "shaders/phong-perVertexColor.vert.spv"
};
static const uint32_t phongPerVertexColorFragmentShaderSpirv[]={
#include "shaders/phong-perVertexColor.frag.spv"
};
static const uint32_t phongPerVertexColorTexturingVertexShaderSpirv[]={
#include "shaders/phong-perVertexColor-texturing.vert.spv"
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
	for(vk::ShaderModule& sm : _vertexShaders) {
		_device->destroy(sm);
		sm = nullptr;
	}
	for(vk::ShaderModule& sm : _fragmentShaders) {
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
						vk::ShaderModuleCreateFlags(),	 // flags
						get<1>(vertexShaderSpirvList[i]),  // codeSize
						get<0>(vertexShaderSpirvList[i])   // pCode
					)
				);
		for(size_t i=0; i<fragmentShaderSpirvList.size(); i++)
			_fragmentShaders[i] =
				_device->createShaderModule(
					vk::ShaderModuleCreateInfo(
						vk::ShaderModuleCreateFlags(),	   // flags
						get<1>(fragmentShaderSpirvList[i]),  // codeSize
						get<0>(fragmentShaderSpirvList[i])   // pCode
					)
				);
		_pipelineLayout =
			_device->createPipelineLayout(
				vk::PipelineLayoutCreateInfo(
					vk::PipelineLayoutCreateFlags(),  // flags
				#if 0 // this will probably come in with texture support
					uint32_t(descriptorSetLayouts.size()),  // setLayoutCount
					descriptorSetLayouts.data(),  // pSetLayouts
				#else
					0,  // setLayoutCount
					nullptr,  // pSetLayouts
				#endif
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
	for(size_t i=0; i<_pipelines.size(); i++) {
		device.destroy(_pipelines[i]);
		_pipelines[i] = nullptr;
	}

	// create new pipelines
	for(size_t i=0; i<_pipelines.size(); i++)
		_pipelines[i] =
			_device->createGraphicsPipeline(
				pipelineCache,  // pipelineCache
				vk::GraphicsPipelineCreateInfo(
					vk::PipelineCreateFlags(),  // flags
					2,  // stageCount
					array<const vk::PipelineShaderStageCreateInfo, 2>{  // pStages
						vk::PipelineShaderStageCreateInfo{
							vk::PipelineShaderStageCreateFlags(),  // flags
							vk::ShaderStageFlagBits::eVertex,	  // stage
							_vertexShaders[i&0x7],  // module
							"main",  // pName
							&specializationInfo,  // pSpecializationInfo
						},
						vk::PipelineShaderStageCreateInfo{
							vk::PipelineShaderStageCreateFlags(),  // flags
							vk::ShaderStageFlagBits::eFragment,	// stage
							_fragmentShaders[i&0x7],  // module
							"main",  // pName
							&specializationInfo,  // pSpecializationInfo
						}
					}.data(),
					&pipelineVertexInputStateCreateInfo,  // pVertexInputState
					&(const vk::PipelineInputAssemblyStateCreateInfo&)vk::PipelineInputAssemblyStateCreateInfo{  // pInputAssemblyState
						vk::PipelineInputAssemblyStateCreateFlags(),  // flags
						vk::PrimitiveTopology::eTriangleList,  // topology
						VK_FALSE  // primitiveRestartEnable
					},
					nullptr, // pTessellationState
					&(const vk::PipelineViewportStateCreateInfo&)vk::PipelineViewportStateCreateInfo{  // pViewportState
						vk::PipelineViewportStateCreateFlags(),  // flags
						1,  // viewportCount
						&(const vk::Viewport&)vk::Viewport(0.f,0.f,float(surfaceExtent.width),float(surfaceExtent.height),0.f,1.f),  // pViewports
						1,  // scissorCount
						&(const vk::Rect2D&)vk::Rect2D(vk::Offset2D(0,0),surfaceExtent)  // pScissors
					},
					&(const vk::PipelineRasterizationStateCreateInfo&)vk::PipelineRasterizationStateCreateInfo{  // pRasterizationState
						vk::PipelineRasterizationStateCreateFlags(),  // flags
						VK_FALSE,  // depthClampEnable
						VK_FALSE,  // rasterizerDiscardEnable
						vk::PolygonMode::eFill,  // polygonMode
						(i&0x08)==0 ? vk::CullModeFlagBits::eNone :  // cullMode (eNone - nothing is discarded)
							vk::CullModeFlagBits::eBack,  // (eBack - back-facing triangles are discarded, eFront - front-facing triangles are discarded)
						(i&0x10) ? vk::FrontFace::eClockwise : vk::FrontFace::eCounterClockwise,  // frontFace
						VK_FALSE,  // depthBiasEnable
						0.f,  // depthBiasConstantFactor
						0.f,  // depthBiasClamp
						0.f,  // depthBiasSlopeFactor
						1.f   // lineWidth
					},
					&(const vk::PipelineMultisampleStateCreateInfo&)vk::PipelineMultisampleStateCreateInfo{  // pMultisampleState
						vk::PipelineMultisampleStateCreateFlags(),  // flags
						vk::SampleCountFlagBits::e1,  // rasterizationSamples
						VK_FALSE,  // sampleShadingEnable
						0.f,	   // minSampleShading
						nullptr,   // pSampleMask
						VK_FALSE,  // alphaToCoverageEnable
						VK_FALSE   // alphaToOneEnable
					},
					&(const vk::PipelineDepthStencilStateCreateInfo&)vk::PipelineDepthStencilStateCreateInfo{  // pDepthStencilState
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
					},
					&(const vk::PipelineColorBlendStateCreateInfo&)vk::PipelineColorBlendStateCreateInfo{  // pColorBlendState
						vk::PipelineColorBlendStateCreateFlags(),  // flags
						VK_FALSE,  // logicOpEnable
						vk::LogicOp::eClear,  // logicOp
						1,  // attachmentCount
						array{  // pAttachments
							vk::PipelineColorBlendAttachmentState{
								VK_FALSE,  // blendEnable
								vk::BlendFactor::eZero,  // srcColorBlendFactor
								vk::BlendFactor::eZero,  // dstColorBlendFactor
								vk::BlendOp::eAdd,	   // colorBlendOp
								vk::BlendFactor::eZero,  // srcAlphaBlendFactor
								vk::BlendFactor::eZero,  // dstAlphaBlendFactor
								vk::BlendOp::eAdd,	   // alphaBlendOp
								vk::ColorComponentFlagBits::eR|vk::ColorComponentFlagBits::eG|
									vk::ColorComponentFlagBits::eB|vk::ColorComponentFlagBits::eA  // colorWriteMask
							},
						}.data(),
						array<float,4>{0.f,0.f,0.f,0.f}  // blendConstants
					},
					nullptr,  // pDynamicState
					_pipelineLayout,  // layout
					renderPass,  // renderPass
					0,  // subpass
					nullptr,  // basePipelineHandle
					-1 // basePipelineIndex
				)
			);
}
