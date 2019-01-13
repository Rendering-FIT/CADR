#ifdef _WIN32
# define VK_USE_PLATFORM_WIN32_KHR
#else
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# define VK_USE_PLATFORM_XLIB_KHR
#endif
#include <vulkan/vulkan.hpp>
#include <array>
#include <iostream>

using namespace std;

// constants
const vk::Extent2D imageExtent(100,100);


// Vulkan instance
// (must be destructed as the last one, at least on Linux, it must be destroyed after display connection)
static vk::UniqueInstance instance;

// Vulkan handles and objects
// (they need to be placed in particular (not arbitrary) order as it gives their destruction order)
static vk::PhysicalDevice physicalDevice;
static uint32_t graphicsQueueFamily;
static vk::UniqueDevice device;
static vk::Queue graphicsQueue;
static vk::UniqueRenderPass renderPass;
static vk::UniqueShaderModule vsModule;
static vk::UniqueShaderModule fsModule;
static vk::UniquePipelineCache pipelineCache;
static vk::UniquePipelineLayout pipelineLayout;
static vk::UniqueImage image;
static vk::UniqueDeviceMemory imageMemory;
static vk::UniqueImageView imageView;
static vk::UniquePipeline pipeline;
static vk::UniqueFramebuffer framebuffer;
static vk::UniqueCommandPool commandPool;
static vk::UniqueCommandBuffer commandBuffer;
static vk::UniqueFence renderingFinishedFence;

// shader code in SPIR-V binary
static const uint32_t vsSpirv[]={
#include "shader.vert.spv"
};
static const uint32_t fsSpirv[]={
#include "shader.frag.spv"
};


/// main function of the application
int main(int,char**)
{
	// catch exceptions
	// (vulkan.hpp fuctions throw if they fail)
	try {

		// Vulkan instance
		instance=
			vk::createInstanceUnique(
				vk::InstanceCreateInfo{
					vk::InstanceCreateFlags(),  // flags
					&(const vk::ApplicationInfo&)vk::ApplicationInfo{
						"CADR Vk Offscreen Rendering", // application name
						VK_MAKE_VERSION(0,0,0),  // application version
						"CADR",                  // engine name
						VK_MAKE_VERSION(0,0,0),  // engine version
						VK_API_VERSION_1_0,      // api version
					},
					0,nullptr,  // no layers
					0,nullptr,  // no extensions
				});

		// find compatible devices
		// (On Windows, all graphics adapters capable of monitor output are usually compatible devices.
		// On Linux X11 platform, only one graphics adapter is compatible device (the one that
		// renders the window).
		vector<vk::PhysicalDevice> deviceList=instance->enumeratePhysicalDevices();
		vector<tuple<vk::PhysicalDevice,uint32_t>> compatibleDevices;
		for(vk::PhysicalDevice pd:deviceList) {

			// select queue for graphics rendering
			vector<vk::QueueFamilyProperties> queueFamilyList=pd.getQueueFamilyProperties();
			for(size_t i=0,c=queueFamilyList.size(); i<c; i++) {
				if(queueFamilyList[i].queueFlags&vk::QueueFlagBits::eGraphics) {
					compatibleDevices.emplace_back(pd,i);
					break;
				}
			}
		}
		cout<<"Compatible devices:"<<endl;
		for(auto& t:compatibleDevices)
			cout<<"   "<<get<0>(t).getProperties().deviceName<<endl;

		// choose device
		if(compatibleDevices.size()>0) {
			auto t=compatibleDevices.front();
			physicalDevice=get<0>(t);
			graphicsQueueFamily=get<1>(t);
		}
		else
			throw runtime_error("No compatible devices.");
		cout<<"Using device:\n"
				"   "<<physicalDevice.getProperties().deviceName<<endl;

		// create device
		device.reset(  // move assignment and physicalDevice.createDeviceUnique() does not work here because of bug in vulkan.hpp until VK_HEADER_VERSION 73 (bug was fixed on 2018-03-05 in vulkan.hpp git). Unfortunately, Ubuntu 18.04 carries still broken vulkan.hpp.
			physicalDevice.createDevice(
				vk::DeviceCreateInfo{
					vk::DeviceCreateFlags(),  // flags
					1,                        // queueCreateInfoCount
					&(const vk::DeviceQueueCreateInfo&)vk::DeviceQueueCreateInfo{  // pQueueCreateInfos
						vk::DeviceQueueCreateFlags(),
						graphicsQueueFamily,
						1,
						&(const float&)1.f,
					},
					0,nullptr,  // no layers
					0,nullptr,  // number of enabled extensions, enabled extension names
					nullptr,    // enabled features
				}
			)
		);

		// get queues
		graphicsQueue=device->getQueue(graphicsQueueFamily,0);

		// render pass
		renderPass=
			device->createRenderPassUnique(
				vk::RenderPassCreateInfo(
					vk::RenderPassCreateFlags(),  // flags
					1,                            // attachmentCount
					&(const vk::AttachmentDescription&)vk::AttachmentDescription(  // pAttachments
						vk::AttachmentDescriptionFlags(),  // flags
						vk::Format::eR8G8B8A8Unorm,        // format
						vk::SampleCountFlagBits::e1,       // samples
						vk::AttachmentLoadOp::eClear,      // loadOp
						vk::AttachmentStoreOp::eStore,     // storeOp
						vk::AttachmentLoadOp::eDontCare,   // stencilLoadOp
						vk::AttachmentStoreOp::eDontCare,  // stencilStoreOp
						vk::ImageLayout::eUndefined,       // initialLayout
						vk::ImageLayout::ePresentSrcKHR    // finalLayout
					),
					1,  // subpassCount
					&(const vk::SubpassDescription&)vk::SubpassDescription(  // pSubpasses
						vk::SubpassDescriptionFlags(),     // flags
						vk::PipelineBindPoint::eGraphics,  // pipelineBindPoint
						0,        // inputAttachmentCount
						nullptr,  // pInputAttachments
						1,        // colorAttachmentCount
						&(const vk::AttachmentReference&)vk::AttachmentReference(  // pColorAttachments
							0,  // attachment
							vk::ImageLayout::eColorAttachmentOptimal  // layout
						),
						nullptr,  // pResolveAttachments
						nullptr,  // pDepthStencilAttachment
						0,        // preserveAttachmentCount
						nullptr   // pPreserveAttachments
					),
					1,  // dependencyCount
					&(const vk::SubpassDependency&)vk::SubpassDependency(  // pDependencies
						VK_SUBPASS_EXTERNAL,   // srcSubpass
						0,                     // dstSubpass
						vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput),  // srcStageMask
						vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput),  // dstStageMask
						vk::AccessFlags(),     // srcAccessMask
						vk::AccessFlags(vk::AccessFlagBits::eColorAttachmentRead|vk::AccessFlagBits::eColorAttachmentWrite),  // dstAccessMask
						vk::DependencyFlags()  // dependencyFlags
					)
				)
			);

		// create shader modules
		vsModule=device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(vsSpirv),  // codeSize
				vsSpirv  // pCode
			)
		);
		fsModule=device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(fsSpirv),  // codeSize
				fsSpirv  // pCode
			)
		);

		// pipeline cache
		pipelineCache=device->createPipelineCacheUnique(
			vk::PipelineCacheCreateInfo(
				vk::PipelineCacheCreateFlags(),  // flags
				0,       // initialDataSize
				nullptr  // pInitialData
			)
		);

		// pipeline layout
		pipelineLayout=device->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				0,       // setLayoutCount
				nullptr, // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			}
		);

		// command pool
		commandPool=
			device->createCommandPoolUnique(
				vk::CommandPoolCreateInfo(
					vk::CommandPoolCreateFlags(),  // flags
					graphicsQueueFamily  // queueFamilyIndex
				)
			);

		// fence
		renderingFinishedFence=
			device->createFenceUnique(
				vk::FenceCreateInfo{
					vk::FenceCreateFlags()  // flags
				}
			);


		// rendered image
		image=
			device->createImageUnique(
				vk::ImageCreateInfo(
					vk::ImageCreateFlags(),       // flags
					vk::ImageType::e2D,           // imageType
					vk::Format::eR8G8B8A8Unorm,   // format
					vk::Extent3D(imageExtent.width,imageExtent.height,1),  // extent
					1,                            // mipLevels
					1,                            // arrayLayers
					vk::SampleCountFlagBits::e1,  // samples
					vk::ImageTiling::eOptimal,    // tiling
					vk::ImageUsageFlagBits::eColorAttachment,  // usage
					vk::SharingMode::eExclusive,  // sharingMode
					0,                            // queueFamilyIndexCount
					nullptr,                      // pQueueFamilyIndices
					vk::ImageLayout::eUndefined   // initialLayout
				)
			);

		// rendered image memory
		imageMemory=
			device->allocateMemoryUnique(
				[](vk::Image image)->vk::MemoryAllocateInfo{
					vk::MemoryRequirements memoryRequirements=device->getImageMemoryRequirements(image);
					vk::PhysicalDeviceMemoryProperties memoryProperties=physicalDevice.getMemoryProperties();
					for(uint32_t i=0; i<memoryProperties.memoryTypeCount; i++)
						if(memoryRequirements.memoryTypeBits&(1<<i))
							if(memoryProperties.memoryTypes[i].propertyFlags&vk::MemoryPropertyFlagBits::eDeviceLocal)
								return vk::MemoryAllocateInfo(
										memoryRequirements.size,  // allocationSize
										i                         // memoryTypeIndex
									);
					throw std::runtime_error("No suitable memory type found for image.");
				}(image.get())
			);
		device->bindImageMemory(
			image.get(),  // image
			imageMemory.get(),  // memory
			0  // memoryOffset
		);

		// image view
		imageView=
			device->createImageViewUnique(
				vk::ImageViewCreateInfo(
					vk::ImageViewCreateFlags(),  // flags
					image.get(),                 // image
					vk::ImageViewType::e2D,      // viewType
					vk::Format::eR8G8B8A8Unorm,  // format
					vk::ComponentMapping(),      // components
					vk::ImageSubresourceRange(   // subresourceRange
						vk::ImageAspectFlagBits::eColor,  // aspectMask
						0,  // baseMipLevel
						1,  // levelCount
						0,  // baseArrayLayer
						1   // layerCount
					)
				)
			);

		// pipeline
		pipeline=device->createGraphicsPipelineUnique(
			pipelineCache.get(),
			vk::GraphicsPipelineCreateInfo(
				vk::PipelineCreateFlags(),  // flags
				2,  // stageCount
				array<const vk::PipelineShaderStageCreateInfo,2>{  // pStages
					vk::PipelineShaderStageCreateInfo{
						vk::PipelineShaderStageCreateFlags(),  // flags
						vk::ShaderStageFlagBits::eVertex,      // stage
						vsModule.get(),  // module
						"main",  // pName
						nullptr  // pSpecializationInfo
					},
					vk::PipelineShaderStageCreateInfo{
						vk::PipelineShaderStageCreateFlags(),  // flags
						vk::ShaderStageFlagBits::eFragment,    // stage
						fsModule.get(),  // module
						"main",  // pName
						nullptr  // pSpecializationInfo
					}
				}.data(),
				&(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{  // pVertexInputState
					vk::PipelineVertexInputStateCreateFlags(),  // flags
					0,        // vertexBindingDescriptionCount
					nullptr,  // pVertexBindingDescriptions
					0,        // vertexAttributeDescriptionCount
					nullptr   // pVertexAttributeDescriptions
				},
				&(const vk::PipelineInputAssemblyStateCreateInfo&)vk::PipelineInputAssemblyStateCreateInfo{  // pInputAssemblyState
					vk::PipelineInputAssemblyStateCreateFlags(),  // flags
					vk::PrimitiveTopology::eTriangleList,  // topology
					VK_FALSE  // primitiveRestartEnable
				},
				nullptr, // pTessellationState
				&(const vk::PipelineViewportStateCreateInfo&)vk::PipelineViewportStateCreateInfo{  // pViewportState
					vk::PipelineViewportStateCreateFlags(),  // flags
					1,  // viewportCount
					&(const vk::Viewport&)vk::Viewport(0.f,0.f,float(imageExtent.width),float(imageExtent.height),0.f,1.f),  // pViewports
					1,  // scissorCount
					&(const vk::Rect2D&)vk::Rect2D(vk::Offset2D(0,0),imageExtent)  // pScissors
				},
				&(const vk::PipelineRasterizationStateCreateInfo&)vk::PipelineRasterizationStateCreateInfo{  // pRasterizationState
					vk::PipelineRasterizationStateCreateFlags(),  // flags
					VK_FALSE,  // depthClampEnable
					VK_FALSE,  // rasterizerDiscardEnable
					vk::PolygonMode::eFill,  // polygonMode
					vk::CullModeFlagBits::eNone,  // cullMode
					vk::FrontFace::eCounterClockwise,  // frontFace
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
					0.f,       // minSampleShading
					nullptr,   // pSampleMask
					VK_FALSE,  // alphaToCoverageEnable
					VK_FALSE   // alphaToOneEnable
				},
				nullptr,  // pDepthStencilState
				&(const vk::PipelineColorBlendStateCreateInfo&)vk::PipelineColorBlendStateCreateInfo{  // pColorBlendState
					vk::PipelineColorBlendStateCreateFlags(),  // flags
					VK_FALSE,  // logicOpEnable
					vk::LogicOp::eClear,  // logicOp
					1,  // attachmentCount
					&(const vk::PipelineColorBlendAttachmentState&)vk::PipelineColorBlendAttachmentState{  // pAttachments
						VK_FALSE,  // blendEnable
						vk::BlendFactor::eZero,  // srcColorBlendFactor
						vk::BlendFactor::eZero,  // dstColorBlendFactor
						vk::BlendOp::eAdd,       // colorBlendOp
						vk::BlendFactor::eZero,  // srcAlphaBlendFactor
						vk::BlendFactor::eZero,  // dstAlphaBlendFactor
						vk::BlendOp::eAdd,       // alphaBlendOp
						vk::ColorComponentFlagBits::eR|vk::ColorComponentFlagBits::eG|
							vk::ColorComponentFlagBits::eB|vk::ColorComponentFlagBits::eA  // colorWriteMask
					},
					array<float,4>{0.f,0.f,0.f,0.f}  // blendConstants
				},
				nullptr,  // pDynamicState
				pipelineLayout.get(),  // layout
				renderPass.get(),  // renderPass
				0,  // subpass
				vk::Pipeline(nullptr),  // basePipelineHandle
				-1 // basePipelineIndex
			)
		);

		// framebuffers
		framebuffer=
			device->createFramebufferUnique(
				vk::FramebufferCreateInfo(
					vk::FramebufferCreateFlags(),  // flags
					renderPass.get(),              // renderPass
					1,&imageView.get(),            // attachmentCount, pAttachments
					imageExtent.width,             // width
					imageExtent.height,            // height
					1  // layers
				)
			);

		// allocate command buffer
		commandBuffer=std::move(
			device->allocateCommandBuffersUnique(
				vk::CommandBufferAllocateInfo(
					commandPool.get(),                 // commandPool
					vk::CommandBufferLevel::ePrimary,  // level
					1                                  // commandBufferCount
				)
			)[0]);

		// record command buffer
		commandBuffer->begin(
			vk::CommandBufferBeginInfo(
				vk::CommandBufferUsageFlagBits::eSimultaneousUse,  // flags
				nullptr  // pInheritanceInfo
			)
		);
		commandBuffer->beginRenderPass(
			vk::RenderPassBeginInfo(
				renderPass.get(),       // renderPass
				framebuffer.get(),      // framebuffer
				vk::Rect2D(vk::Offset2D(0,0),imageExtent),  // renderArea
				1,                      // clearValueCount
				&(const vk::ClearValue&)vk::ClearValue(vk::ClearColorValue(array<float,4>{0.f,0.f,0.f,1.f}))  // pClearValues
			),
			vk::SubpassContents::eInline
		);
		commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics,pipeline.get());  // bind pipeline
		commandBuffer->draw(3,1,0,0);  // draw single triangle
		commandBuffer->endRenderPass();
		commandBuffer->end();


		// submit work
		graphicsQueue.submit(
			vk::ArrayProxy<const vk::SubmitInfo>(
				1,
				&(const vk::SubmitInfo&)vk::SubmitInfo(
					0,nullptr,                       // waitSemaphoreCount, pWaitSemaphores
					&(const vk::PipelineStageFlags&)vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput),  // pWaitDstStageMask
					1,&commandBuffer.get(),          // commandBufferCount, pCommandBuffers
					0,nullptr                        // signalSemaphoreCount, pSignalSemaphores
				)
			),
			renderingFinishedFence.get()  // fence
		);

		vk::Result r=device->waitForFences(
			1,&renderingFinishedFence.get(),  // fenceCount, pFences
			VK_TRUE,  // waitAll
			3e9       // timeout (3s)
		);
		if(r==vk::Result::eTimeout)
			throw std::runtime_error("GPU timeout. Task is probably hanging.");

	// catch exceptions
	} catch(vk::Error &e) {
		cout<<"Failed because of Vulkan exception: "<<e.what()<<endl;
	} catch(exception &e) {
		cout<<"Failed because of exception: "<<e.what()<<endl;
	} catch(...) {
		cout<<"Failed because of unspecified exception."<<endl;
	}

	return 0;
}
