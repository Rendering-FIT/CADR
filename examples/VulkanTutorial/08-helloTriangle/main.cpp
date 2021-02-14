#include <vulkan/vulkan.hpp>
#include <array>
#include <fstream>
#include <iostream>

using namespace std;

// constants
const vk::Extent2D imageExtent(128,128);


// Vulkan instance
// (must be destructed as the last one)
static vk::UniqueInstance instance;

// Vulkan handles and objects
// (they need to be placed in particular (not arbitrary) order;
// this is because of their destruction order from bottom to up)
static vk::PhysicalDevice physicalDevice;
static uint32_t graphicsQueueFamily;
static vk::UniqueDevice device;
static vk::Queue graphicsQueue;
static vk::UniqueRenderPass renderPass;
static vk::UniqueShaderModule vsModule;
static vk::UniqueShaderModule fsModule;
static vk::UniquePipelineCache pipelineCache;
static vk::UniquePipelineLayout pipelineLayout;
static vk::UniqueImage framebufferImage;
static vk::UniqueImage hostVisibleImage;
static vk::UniqueDeviceMemory framebufferImageMemory;
static vk::UniqueDeviceMemory hostVisibleImageMemory;
static vk::UniqueImageView frameImageView;
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
						"08-helloTriangle",      // application name
						VK_MAKE_VERSION(0,0,0),  // application version
						nullptr,                 // engine name
						VK_MAKE_VERSION(0,0,0),  // engine version
						VK_API_VERSION_1_0,      // api version
					},
					0,nullptr,  // no layers
					0,nullptr,  // no extensions
				});

		// find compatible devices
		// (the device must have a queue supporting graphics operations)
		vector<vk::PhysicalDevice> deviceList=instance->enumeratePhysicalDevices();
		vector<tuple<vk::PhysicalDevice,uint32_t>> compatibleDevices;
		for(vk::PhysicalDevice pd:deviceList) {

			// select queue for graphics rendering
			vector<vk::QueueFamilyProperties> queueFamilyList=pd.getQueueFamilyProperties();
			for(uint32_t i=0,c=uint32_t(queueFamilyList.size()); i<c; i++) {
				if(queueFamilyList[i].queueFlags & vk::QueueFlagBits::eGraphics) {
					compatibleDevices.emplace_back(pd,i);
					break;
				}
			}
		}

		// print devices
		cout<<"Vulkan devices:"<<endl;
		for(vk::PhysicalDevice pd:deviceList)
			cout<<"   "<<pd.getProperties().deviceName<<endl;
		cout<<"Compatible devices:"<<endl;
		for(auto& t:compatibleDevices)
			cout<<"   "<<get<0>(t).getProperties().deviceName<<endl;

		// choose device
		if(compatibleDevices.empty())
			throw runtime_error("No compatible devices.");
		physicalDevice=get<0>(compatibleDevices.front());
		graphicsQueueFamily=get<1>(compatibleDevices.front());
		cout<<"Using device:\n"
		      "   "<<physicalDevice.getProperties().deviceName<<endl;

		// create device
		device=
			physicalDevice.createDeviceUnique(
				vk::DeviceCreateInfo{
					vk::DeviceCreateFlags(),  // flags
					1,                        // queueCreateInfoCount
					array{                    // pQueueCreateInfos
						vk::DeviceQueueCreateInfo{
							vk::DeviceQueueCreateFlags(),  // flags
							graphicsQueueFamily,  // queueFamilyIndex
							1,                    // queueCount
							&(const float&)1.f,   // pQueuePriorities
						},
					}.data(),
					0,nullptr,  // no layers
					0,nullptr,  // number of enabled extensions, enabled extension names
					nullptr,    // enabled features
				}
			);

		// get queues
		graphicsQueue=device->getQueue(graphicsQueueFamily,0);


		// render pass
		renderPass=
			device->createRenderPassUnique(
				vk::RenderPassCreateInfo(
					vk::RenderPassCreateFlags(),  // flags
					1,                            // attachmentCount
					array{  // pAttachments
						vk::AttachmentDescription(
							vk::AttachmentDescriptionFlags(),  // flags
							vk::Format::eR8G8B8A8Unorm,        // format
							vk::SampleCountFlagBits::e1,       // samples
							vk::AttachmentLoadOp::eClear,      // loadOp
							vk::AttachmentStoreOp::eStore,     // storeOp
							vk::AttachmentLoadOp::eDontCare,   // stencilLoadOp
							vk::AttachmentStoreOp::eDontCare,  // stencilStoreOp
							vk::ImageLayout::eUndefined,       // initialLayout
							vk::ImageLayout::eTransferSrcOptimal  // finalLayout
						),
					}.data(),
					1,  // subpassCount
					array{  // pSubpasses
						vk::SubpassDescription(
							vk::SubpassDescriptionFlags(),     // flags
							vk::PipelineBindPoint::eGraphics,  // pipelineBindPoint
							0,        // inputAttachmentCount
							nullptr,  // pInputAttachments
							1,        // colorAttachmentCount
							array{    // pColorAttachments
								vk::AttachmentReference(
									0,  // attachment
									vk::ImageLayout::eColorAttachmentOptimal  // layout
								),
							}.data(),
							nullptr,  // pResolveAttachments
							nullptr,  // pDepthStencilAttachment
							0,        // preserveAttachmentCount
							nullptr   // pPreserveAttachments
						),
					}.data(),
					1,  // dependencyCount
					array{  // pDependencies
						vk::SubpassDependency(
							0,  // srcSubpass
							VK_SUBPASS_EXTERNAL,  // dstSubpass
							vk::PipelineStageFlagBits::eColorAttachmentOutput, // srcStageMask
							vk::PipelineStageFlagBits::eTransfer,     // dstStageMask
							vk::AccessFlagBits::eColorAttachmentWrite,  // srcAccessMask
							vk::AccessFlagBits::eTransferRead,  // dstAccessMask
							vk::DependencyFlags()  // dependencyFlags
						),
					}.data()
				)
			);

		// images
		framebufferImage=
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
					vk::ImageUsageFlagBits::eColorAttachment|vk::ImageUsageFlagBits::eTransferSrc,  // usage
					vk::SharingMode::eExclusive,  // sharingMode
					0,                            // queueFamilyIndexCount
					nullptr,                      // pQueueFamilyIndices
					vk::ImageLayout::eUndefined   // initialLayout
				)
			);
		hostVisibleImage=
			device->createImageUnique(
				vk::ImageCreateInfo(
					vk::ImageCreateFlags(),       // flags
					vk::ImageType::e2D,           // imageType
					vk::Format::eR8G8B8A8Unorm,   // format
					vk::Extent3D(imageExtent.width,imageExtent.height,1),  // extent
					1,                            // mipLevels
					1,                            // arrayLayers
					vk::SampleCountFlagBits::e1,  // samples
					vk::ImageTiling::eLinear,     // tiling
					vk::ImageUsageFlagBits::eTransferDst,  // usage
					vk::SharingMode::eExclusive,  // sharingMode
					0,                            // queueFamilyIndexCount
					nullptr,                      // pQueueFamilyIndices
					vk::ImageLayout::eUndefined   // initialLayout
				)
			);

		// memory for images
		auto allocateMemory=
			[](vk::Image image,vk::MemoryPropertyFlags requiredFlags)->vk::UniqueDeviceMemory{
				vk::MemoryRequirements memoryRequirements=device->getImageMemoryRequirements(image);
				vk::PhysicalDeviceMemoryProperties memoryProperties=physicalDevice.getMemoryProperties();
				for(uint32_t i=0; i<memoryProperties.memoryTypeCount; i++)
					if(memoryRequirements.memoryTypeBits&(1<<i))
						if((memoryProperties.memoryTypes[i].propertyFlags&requiredFlags)==requiredFlags)
							return
								device->allocateMemoryUnique(
									vk::MemoryAllocateInfo(
										memoryRequirements.size,  // allocationSize
										i                         // memoryTypeIndex
									)
								);
				throw std::runtime_error("No suitable memory type found for image.");
			};
		framebufferImageMemory=allocateMemory(framebufferImage.get(),vk::MemoryPropertyFlagBits::eDeviceLocal);
		hostVisibleImageMemory=allocateMemory(hostVisibleImage.get(),vk::MemoryPropertyFlagBits::eHostVisible|
		                                                             vk::MemoryPropertyFlagBits::eHostCached);
		device->bindImageMemory(
			framebufferImage.get(),        // image
			framebufferImageMemory.get(),  // memory
			0                              // memoryOffset
		);
		device->bindImageMemory(
			hostVisibleImage.get(),        // image
			hostVisibleImageMemory.get(),  // memory
			0                              // memoryOffset
		);

		// image view
		frameImageView=
			device->createImageViewUnique(
				vk::ImageViewCreateInfo(
					vk::ImageViewCreateFlags(),  // flags
					framebufferImage.get(),      // image
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

		// framebuffers
		framebuffer=
			device->createFramebufferUnique(
				vk::FramebufferCreateInfo(
					vk::FramebufferCreateFlags(),  // flags
					renderPass.get(),              // renderPass
					1,&frameImageView.get(),       // attachmentCount, pAttachments
					imageExtent.width,             // width
					imageExtent.height,            // height
					1  // layers
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

		// pipeline
		tie(ignore,pipeline)=
			device->createGraphicsPipelineUnique(
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
			).asTuple();


		// command pool
		commandPool=
			device->createCommandPoolUnique(
				vk::CommandPoolCreateInfo(
					vk::CommandPoolCreateFlags(),  // flags
					graphicsQueueFamily  // queueFamilyIndex
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

		// begin command buffer
		commandBuffer->begin(
			vk::CommandBufferBeginInfo(
				vk::CommandBufferUsageFlagBits::eOneTimeSubmit,  // flags
				nullptr  // pInheritanceInfo
			)
		);

		// begin render pass
		commandBuffer->beginRenderPass(
			vk::RenderPassBeginInfo(
				renderPass.get(),   // renderPass
				framebuffer.get(),  // framebuffer
				vk::Rect2D(vk::Offset2D(0,0),imageExtent),  // renderArea
				1,      // clearValueCount
				array{  // pClearValues
					vk::ClearValue(array<float,4>{0.f,0.f,0.f,1.f}),
				}.data()
			),
			vk::SubpassContents::eInline
		);

		// rendering commands
		commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics,pipeline.get());  // bind pipeline
		commandBuffer->draw(3,1,0,0);  // draw single triangle

		// end render pass
		commandBuffer->endRenderPass();


		// hostVisibleImage layout to TransferDstOptimal
		commandBuffer->pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,  // srcStageMask
			vk::PipelineStageFlagBits::eTransfer,   // dstStageMask
			vk::DependencyFlags(),  // dependencyFlags
			nullptr,  // memoryBarriers
			nullptr,  // bufferMemoryBarriers
			vk::ArrayProxy<const vk::ImageMemoryBarrier>{  // imageMemoryBarriers
				vk::ImageMemoryBarrier{
					vk::AccessFlags(),                     // srcAccessMask
					vk::AccessFlagBits::eTransferWrite,    // dstAccessMask
					vk::ImageLayout::eUndefined,           // oldLayout
					vk::ImageLayout::eGeneral,             // newLayout
					0,                          // srcQueueFamilyIndex
					0,                          // dstQueueFamilyIndex
					hostVisibleImage.get(),     // image
					vk::ImageSubresourceRange{  // subresourceRange
						vk::ImageAspectFlagBits::eColor,  // aspectMask
						0,  // baseMipLevel
						1,  // levelCount
						0,  // baseArrayLayer
						1   // layerCount
					}
				}
			}
		);

		// copy framebufferImage to hostVisibleImage
		commandBuffer->copyImage(
			framebufferImage.get(),vk::ImageLayout::eTransferSrcOptimal,  // srcImage,srcImageLayout
			hostVisibleImage.get(),vk::ImageLayout::eGeneral,  // dstImage,dstImageLayout
			vk::ImageCopy(
				vk::ImageSubresourceLayers(  // srcSubresource
					vk::ImageAspectFlagBits::eColor,  // aspectMask
					0,  // mipLevel
					0,  // baseArrayLayer
					1   // layerCount
				),
				vk::Offset3D(0,0,0),         // srcOffset
				vk::ImageSubresourceLayers(  // dstSubresource
					vk::ImageAspectFlagBits::eColor,  // aspectMask
					0,  // mipLevel
					0,  // baseArrayLayer
					1   // layerCount
				),
				vk::Offset3D(0,0,0),         // dstOffset
				vk::Extent3D(imageExtent.width,imageExtent.height,1)  // extent
			)
		);

		// end command buffer
		commandBuffer->end();


		// fence
		renderingFinishedFence=
			device->createFenceUnique(
				vk::FenceCreateInfo{
					vk::FenceCreateFlags()  // flags
				}
			);

		// submit work
		graphicsQueue.submit(
			vk::SubmitInfo(  // submits
				0,nullptr,nullptr,       // waitSemaphoreCount, pWaitSemaphores, pWaitDstStageMask
				1,&commandBuffer.get(),  // commandBufferCount, pCommandBuffers
				0,nullptr                // signalSemaphoreCount, pSignalSemaphores
			),
			renderingFinishedFence.get()  // fence
		);

		// wait for the work
		vk::Result r=device->waitForFences(
			renderingFinishedFence.get(),  // fences (vk::ArrayProxy)
			VK_TRUE,       // waitAll
			uint64_t(3e9)  // timeout (3s)
		);
		if(r==vk::Result::eTimeout)
			throw std::runtime_error("GPU timeout. Task is probably hanging.");


		// map memory
		struct MappedMemoryDeleter { void operator()(void*) { device->unmapMemory(hostVisibleImageMemory.get()); } } mappedMemoryDeleter;
		unique_ptr<void,MappedMemoryDeleter> m(
			device->mapMemory(hostVisibleImageMemory.get(),0,VK_WHOLE_SIZE,vk::MemoryMapFlags()),  // pointer
			mappedMemoryDeleter  // deleter
		);

		// invalidate caches to fetch a new content
		// (this is required as we might be using non-coherent memory, see vk::MemoryPropertyFlagBits::eHostCoherent)
		device->invalidateMappedMemoryRanges(
			vk::MappedMemoryRange(
				hostVisibleImageMemory.get(),  // memory
				0,  // offset
				VK_WHOLE_SIZE  // size
			)
		);

		// get image memory layout
		vk::SubresourceLayout hostImageLayout=
			device->getImageSubresourceLayout(
				hostVisibleImage.get(),  // image
				vk::ImageSubresource{    // subresource
					vk::ImageAspectFlagBits::eColor,  // aspectMask
					0,  // mipLevel
					0   // arrayLayer
				}
			);


		// open the output file
		cout<<"Writing \"image.bmp\"..."<<endl;
		fstream s("image.bmp",fstream::out|fstream::binary);

		// write BitmapFileHeader
		struct BitmapFileHeader {
			uint16_t type = 0x4d42;
			uint16_t sizeLo;
			uint16_t sizeHi;
			uint16_t reserved1 = 0;
			uint16_t reserved2 = 0;
			uint16_t offsetLo;
			uint16_t offsetHi;
		};
		static_assert(sizeof(BitmapFileHeader)==14,"Wrong alignment of BitmapFileHeader members.");

		uint32_t imageDataSize=imageExtent.width*imageExtent.height*4;
		uint32_t fileSize=imageDataSize+14+40+2;
		BitmapFileHeader bitmapFileHeader = {
			0x4d42,
			uint16_t(fileSize&0xffff),
			uint16_t(fileSize>>16),
			0,0,
			14+40+2,  // equal to sum of sizeof(BitmapFileHeader), sizeof(BitmapInfoHeader) and 2 (as alignment)
			0
		};
		s.write(reinterpret_cast<char*>(&bitmapFileHeader),sizeof(BitmapFileHeader));

		// write BitmapInfoHeader
		struct BitmapInfoHeader {
			uint32_t size = 40;
			int32_t  width;
			int32_t  height;
			uint16_t numPlanes = 1;
			uint16_t bpp = 32;
			uint32_t compression = 0;  // 0 - no compression
			uint32_t imageDataSize;
			int32_t  xPixelsPerMeter;
			int32_t  yPixelsPerMeter;
			uint32_t numColorsInPalette = 0;  // no colors in color palette
			uint32_t numImportantColors = 0;  // all colors are important
		};
		static_assert(sizeof(BitmapInfoHeader)==40,"Wrong size of BitmapInfoHeader.");

		BitmapInfoHeader bitmapInfoHeader = {
			40,
			int32_t(imageExtent.width),
			-int32_t(imageExtent.height),
			1,32,0,
			imageDataSize,
			2835,2835,  // roughly 72 DPI
			0,0
		};
		s.write(reinterpret_cast<char*>(&bitmapInfoHeader),sizeof(BitmapInfoHeader));
		s.write(array<char,2>{'\0','\0'}.data(),2);

		// write image data,
		// line by line
		const char* linePtr=reinterpret_cast<char*>(m.get());
		char b[4];
		for(auto y=imageExtent.height; y>0; y--) {
			for(size_t i=0,e=imageExtent.width*4; i<e; i+=4) {
				b[0]=linePtr[i+2];
				b[1]=linePtr[i+1];
				b[2]=linePtr[i+0];
				b[3]=linePtr[i+3];
				s.write(b,4);
			}
			linePtr+=hostImageLayout.rowPitch;
		}
		cout<<"Done."<<endl;

	// catch exceptions
	} catch(vk::Error &e) {
		cout<<"Failed because of Vulkan exception: "<<e.what()<<endl;
	} catch(exception &e) {
		cout<<"Failed because of exception: "<<e.what()<<endl;
	} catch(...) {
		cout<<"Failed because of unspecified exception."<<endl;
	}

	// wait device idle, particularly, if there was an exception and device is busy
	// (device must be idle before destructors of buffers and other stuff are called)
	if(device)
		device->waitIdle();

	return 0;
}
