#include <QCoreApplication>
#include <QResizeEvent>
#ifndef _WIN32
# include <QX11Info>
# include <QBackingStore>
#endif
#include "VulkanWindow.h"
#include <vector>
#include <iostream>

using namespace std;


// shader code in SPIR-V binary
static const uint32_t vsSpirv[]={
#include "shader.vert.spv"
};
static const uint32_t fsSpirv[]={
#include "shader.frag.spv"
};



/// Allocate Vulkan resources.
void VulkanWindow::showEvent(QShowEvent* e)
{
	inherited::showEvent(e);

	if(_surface)
		return;

	// create surface
	vk::Instance instance(vulkanInstance()->vkInstance());
#ifdef _WIN32
	_surface=instance.createWin32SurfaceKHRUnique(vk::Win32SurfaceCreateInfoKHR(vk::Win32SurfaceCreateFlagsKHR(),GetModuleHandle(NULL),(HWND)winId()));
#else
	_surface=instance.createXlibSurfaceKHRUnique(vk::XlibSurfaceCreateInfoKHR(vk::XlibSurfaceCreateFlagsKHR(),QX11Info::display(),winId()));
#endif

	// find compatible devices
	// (On Windows, all graphics adapters capable of monitor output are usually compatible devices.
	// On Linux X11 platform, only one graphics adapter is compatible device (the one that
	// renders the window).
	vector<vk::PhysicalDevice> deviceList=instance.enumeratePhysicalDevices();
	vector<tuple<vk::PhysicalDevice,uint32_t>> compatibleDevicesSingleQueue;
	vector<tuple<vk::PhysicalDevice,uint32_t,uint32_t>> compatibleDevicesTwoQueues;
	for(vk::PhysicalDevice pd:deviceList) {

		// skip devices without VK_KHR_swapchain
		auto extensionList=pd.enumerateDeviceExtensionProperties();
		for(vk::ExtensionProperties& e:extensionList)
			if(strcmp(e.extensionName,"VK_KHR_swapchain")==0)
				goto swapchainSupported;
		continue;
		swapchainSupported:

		// skip devices without surface formats and presentation modes
		uint32_t formatCount;
		vk::createResultValue(pd.getSurfaceFormatsKHR(_surface.get(),&formatCount,nullptr),VULKAN_HPP_NAMESPACE_STRING"::PhysicalDevice::getSurfaceFormatsKHR");
		uint32_t presentationModeCount;
		vk::createResultValue(pd.getSurfacePresentModesKHR(_surface.get(),&presentationModeCount,nullptr),VULKAN_HPP_NAMESPACE_STRING"::PhysicalDevice::getSurfacePresentModesKHR");
		if(formatCount==0||presentationModeCount==0)
			continue;

		// select queues (for graphics rendering and for presentation)
		uint32_t graphicsQueueFamily=UINT32_MAX;
		uint32_t presentationQueueFamily=UINT32_MAX;
		vector<vk::QueueFamilyProperties> queueFamilyList=pd.getQueueFamilyProperties();
		vector<bool> presentationSupport;
		presentationSupport.reserve(queueFamilyList.size());
		uint32_t i=0;
		for(auto it=queueFamilyList.begin(); it!=queueFamilyList.end(); it++,i++) {
			bool p=pd.getSurfaceSupportKHR(i,_surface.get())!=0;
			if(it->queueFlags&vk::QueueFlagBits::eGraphics) {
				if(p) {
					compatibleDevicesSingleQueue.emplace_back(pd,i);
					goto nextDevice;
				}
				presentationSupport.push_back(p);
				if(graphicsQueueFamily==UINT32_MAX)
					graphicsQueueFamily=i;
			}
			else {
				presentationSupport.push_back(p);
				if(p)
					if(presentationQueueFamily==UINT32_MAX)
						presentationQueueFamily=i;
			}
		}
		compatibleDevicesTwoQueues.emplace_back(pd,graphicsQueueFamily,presentationQueueFamily);
		nextDevice:;
	}
	cout<<"Compatible devices:"<<endl;
	for(auto& t:compatibleDevicesSingleQueue)
		cout<<"   "<<get<0>(t).getProperties().deviceName<<endl;
	for(auto& t:compatibleDevicesTwoQueues)
		cout<<"   "<<get<0>(t).getProperties().deviceName<<endl;

	// choose device
	if(compatibleDevicesSingleQueue.size()>0) {
		auto t=compatibleDevicesSingleQueue.front();
		physicalDevice=get<0>(t);
		graphicsQueueFamily=get<1>(t);
		presentationQueueFamily=graphicsQueueFamily;
	}
	else if(compatibleDevicesTwoQueues.size()>0) {
		auto t=compatibleDevicesTwoQueues.front();
		physicalDevice=get<0>(t);
		graphicsQueueFamily=get<1>(t);
		presentationQueueFamily=get<2>(t);
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
				compatibleDevicesSingleQueue.size()>0?uint32_t(1):uint32_t(2),  // queueCreateInfoCount
				array<const vk::DeviceQueueCreateInfo,2>{  // pQueueCreateInfos
					vk::DeviceQueueCreateInfo{
						vk::DeviceQueueCreateFlags(),
						graphicsQueueFamily,
						1,
						&(const float&)1.f,
					},
					vk::DeviceQueueCreateInfo{
						vk::DeviceQueueCreateFlags(),
						presentationQueueFamily,
						1,
						&(const float&)1.f,
					}
				}.data(),
				0,nullptr,  // no layers
				1,          // number of enabled extensions
				array<const char*,1>{"VK_KHR_swapchain"}.data(),  // enabled extension names
				nullptr,    // enabled features
			}
		)
	);

	// get queues
	graphicsQueue=device->getQueue(graphicsQueueFamily,0);
	presentationQueue=device->getQueue(presentationQueueFamily,0);

	// choose surface format
	vector<vk::SurfaceFormatKHR> surfaceFormats=physicalDevice.getSurfaceFormatsKHR(_surface.get());
	const vk::SurfaceFormatKHR wantedSurfaceFormat{vk::Format::eB8G8R8A8Unorm,vk::ColorSpaceKHR::eSrgbNonlinear};
	chosenSurfaceFormat=
		surfaceFormats.size()==1&&surfaceFormats[0].format==vk::Format::eUndefined
			?wantedSurfaceFormat
			:std::find(surfaceFormats.begin(),surfaceFormats.end(),
			           wantedSurfaceFormat)!=surfaceFormats.end()
				?wantedSurfaceFormat
				:surfaceFormats[0];
	depthFormat=[](const vk::PhysicalDevice physicalDevice){
		for(vk::Format f:array<vk::Format,3>{vk::Format::eD32Sfloat,vk::Format::eD32SfloatS8Uint,vk::Format::eD24UnormS8Uint}) {
			vk::FormatProperties p=physicalDevice.getFormatProperties(f);
			if(p.optimalTilingFeatures&vk::FormatFeatureFlagBits::eDepthStencilAttachment)
				return f;
		}
		throw std::runtime_error("No suitable depth buffer format.");
	}(physicalDevice);

	// render pass
	renderPass=
		device->createRenderPassUnique(
			vk::RenderPassCreateInfo(
				vk::RenderPassCreateFlags(),  // flags
				2,                            // attachmentCount
				array<const vk::AttachmentDescription,2>{  // pAttachments
					vk::AttachmentDescription{  // color attachment
						vk::AttachmentDescriptionFlags(),  // flags
						chosenSurfaceFormat.format,        // format
						vk::SampleCountFlagBits::e1,       // samples
						vk::AttachmentLoadOp::eClear,      // loadOp
						vk::AttachmentStoreOp::eStore,     // storeOp
						vk::AttachmentLoadOp::eDontCare,   // stencilLoadOp
						vk::AttachmentStoreOp::eDontCare,  // stencilStoreOp
						vk::ImageLayout::eUndefined,       // initialLayout
						vk::ImageLayout::ePresentSrcKHR    // finalLayout
					},
					vk::AttachmentDescription{  // depth attachment
						vk::AttachmentDescriptionFlags(),  // flags
						depthFormat,                       // format
						vk::SampleCountFlagBits::e1,       // samples
						vk::AttachmentLoadOp::eClear,      // loadOp
						vk::AttachmentStoreOp::eDontCare,  // storeOp
						vk::AttachmentLoadOp::eDontCare,   // stencilLoadOp
						vk::AttachmentStoreOp::eDontCare,  // stencilStoreOp
						vk::ImageLayout::eUndefined,       // initialLayout
						vk::ImageLayout::eDepthStencilAttachmentOptimal  // finalLayout
					}
				}.data(),
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
					&(const vk::AttachmentReference&)vk::AttachmentReference(  // pDepthStencilAttachment
						1,  // attachment
						vk::ImageLayout::eDepthStencilAttachmentOptimal  // layout
					),
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

	// semaphores
	imageAvailableSemaphore=
		device->createSemaphoreUnique(
			vk::SemaphoreCreateInfo(
				vk::SemaphoreCreateFlags()  // flags
			)
		);
	renderFinishedSemaphore=
		device->createSemaphoreUnique(
			vk::SemaphoreCreateInfo(
				vk::SemaphoreCreateFlags()  // flags
			)
		);
}


/// Recreate swapchain and pipeline.
void VulkanWindow::resizeEvent(QResizeEvent* e)
{
	cout<<"Resize "<<e->size().width()<<"x"<<e->size().height()<<endl;
	inherited::resizeEvent(e);

	// Linux (xcb platform) needs something like "touch" the resized window (?). Anyway, it makes the things work.
#ifndef _WIN32
	QBackingStore bs(this);
	bs.resize(QSize(1,1));
	bs.flush(QRect(0, 0, width(), height()));
#endif

	// stop device and clear resources
	device->waitIdle();
	commandBuffers.clear();
	framebuffers.clear();
	depthImage.reset();
	depthImageMemory.reset();
	depthImageView.reset();
	pipeline.reset();
	swapchainImageViews.clear();
	swapchain.reset();

	// currentSurfaceExtent
	vk::SurfaceCapabilitiesKHR surfaceCapabilities=physicalDevice.getSurfaceCapabilitiesKHR(_surface.get());
	const QSize& windowSize=e->size();
	_currentSurfaceExtent=(surfaceCapabilities.currentExtent.width!=std::numeric_limits<uint32_t>::max())
			?surfaceCapabilities.currentExtent
			:vk::Extent2D{max(min(uint32_t(windowSize.width()),surfaceCapabilities.maxImageExtent.width),surfaceCapabilities.minImageExtent.width),
			              max(min(uint32_t(windowSize.height()),surfaceCapabilities.maxImageExtent.height),surfaceCapabilities.minImageExtent.height)};

	// do not try to create swapchain if surface extent is 0,0
	// (0,0 is returned on some platforms (for instance Windows) when window is minimized.
	// The creation of swapchain then raises an exception, for instance vk::Result::eErrorOutOfDeviceMemory on Windows)
	if(_currentSurfaceExtent.width==0||_currentSurfaceExtent.height==0)
		return;

	// create swapchain
	swapchain=
		device->createSwapchainKHRUnique(
			vk::SwapchainCreateInfoKHR(
				vk::SwapchainCreateFlagsKHR(),   // flags
				_surface.get(),                  // surface
				surfaceCapabilities.maxImageCount==0  // minImageCount
					?surfaceCapabilities.minImageCount+1
					:min(surfaceCapabilities.maxImageCount,surfaceCapabilities.minImageCount+1),
				chosenSurfaceFormat.format,      // imageFormat
				chosenSurfaceFormat.colorSpace,  // imageColorSpace
				_currentSurfaceExtent,  // imageExtent
				1,  // imageArrayLayers
				vk::ImageUsageFlagBits::eColorAttachment,  // imageUsage
				(graphicsQueueFamily==presentationQueueFamily)?vk::SharingMode::eExclusive:vk::SharingMode::eConcurrent, // imageSharingMode
				(graphicsQueueFamily==presentationQueueFamily)?uint32_t(0):uint32_t(2),  // queueFamilyIndexCount
				(graphicsQueueFamily==presentationQueueFamily)?nullptr:array<uint32_t,2>{graphicsQueueFamily,presentationQueueFamily}.data(),  // pQueueFamilyIndices
				surfaceCapabilities.currentTransform,    // preTransform
				vk::CompositeAlphaFlagBitsKHR::eOpaque,  // compositeAlpha
				[](vector<vk::PresentModeKHR>&& modes){  // presentMode
						return find(modes.begin(),modes.end(),vk::PresentModeKHR::eMailbox)!=modes.end()
							?vk::PresentModeKHR::eMailbox
							:vk::PresentModeKHR::eFifo; // fifo is guaranteed to be supported
					}(physicalDevice.getSurfacePresentModesKHR(_surface.get())),
				VK_TRUE,         // clipped
				swapchain.get()  // oldSwapchain
			)
		);

	// swapchain images and image views
	vector<vk::Image> swapchainImages=device->getSwapchainImagesKHR(swapchain.get());
	swapchainImageViews.reserve(swapchainImages.size());
	for(vk::Image image:swapchainImages)
		swapchainImageViews.emplace_back(
			device->createImageViewUnique(
				vk::ImageViewCreateInfo(
					vk::ImageViewCreateFlags(),  // flags
					image,                       // image
					vk::ImageViewType::e2D,      // viewType
					chosenSurfaceFormat.format,  // format
					vk::ComponentMapping(),      // components
					vk::ImageSubresourceRange(   // subresourceRange
						vk::ImageAspectFlagBits::eColor,  // aspectMask
						0,  // baseMipLevel
						1,  // levelCount
						0,  // baseArrayLayer
						1   // layerCount
					)
				)
			)
		);

	// depth image
	depthImage=
		device->createImageUnique(
			vk::ImageCreateInfo(
				vk::ImageCreateFlags(),  // flags
				vk::ImageType::e2D,      // imageType
				depthFormat,             // format
				vk::Extent3D(_currentSurfaceExtent.width,_currentSurfaceExtent.height,1),  // extent
				1,                       // mipLevels
				1,                       // arrayLayers
				vk::SampleCountFlagBits::e1,  // samples
				vk::ImageTiling::eOptimal,    // tiling
				vk::ImageUsageFlagBits::eDepthStencilAttachment,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr,                      // pQueueFamilyIndices
				vk::ImageLayout::eUndefined   // initialLayout
			)
		);

	// depth image memory
	depthImageMemory=
		device->allocateMemoryUnique(
			[](vk::Image depthImage,vk::Device device,vk::PhysicalDevice physicalDevice)->vk::MemoryAllocateInfo{
				vk::MemoryRequirements memoryRequirements=device.getImageMemoryRequirements(depthImage);
				vk::PhysicalDeviceMemoryProperties memoryProperties=physicalDevice.getMemoryProperties();
				for(uint32_t i=0; i<memoryProperties.memoryTypeCount; i++)
					if(memoryRequirements.memoryTypeBits&(1<<i))
						if(memoryProperties.memoryTypes[i].propertyFlags&vk::MemoryPropertyFlagBits::eDeviceLocal)
							return vk::MemoryAllocateInfo(
									memoryRequirements.size,  // allocationSize
									i                         // memoryTypeIndex
								);
				throw std::runtime_error("No suitable memory type found for depth buffer.");
			}(depthImage.get(),device.get(),physicalDevice)
		);
	device->bindImageMemory(
		depthImage.get(),  // image
		depthImageMemory.get(),  // memory
		0  // memoryOffset
	);

	// depth image view
	depthImageView=
		device->createImageViewUnique(
			vk::ImageViewCreateInfo(
				vk::ImageViewCreateFlags(),  // flags
				depthImage.get(),            // image
				vk::ImageViewType::e2D,      // viewType
				depthFormat,                 // format
				vk::ComponentMapping(),      // components
				vk::ImageSubresourceRange(   // subresourceRange
					vk::ImageAspectFlagBits::eDepth,  // aspectMask
					0,  // baseMipLevel
					1,  // levelCount
					0,  // baseArrayLayer
					1   // layerCount
				)
			)
		);

	// depth image layout
	vk::UniqueCommandBuffer cb=std::move(
		device->allocateCommandBuffersUnique(
			vk::CommandBufferAllocateInfo(
				commandPool.get(),  // commandPool
				vk::CommandBufferLevel::ePrimary,  // level
				1  // commandBufferCount
			)
		)[0]);
	cb->begin(
		vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,  // flags
			nullptr  // pInheritanceInfo
		)
	);
	cb->pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe,  // srcStageMask
		vk::PipelineStageFlagBits::eEarlyFragmentTests,  // dstStageMask
		vk::DependencyFlags(),  // dependencyFlags
		0,nullptr,  // memoryBarrierCount,pMemoryBarriers
		0,nullptr,  // bufferMemoryBarrierCount,pBufferMemoryBarriers
		1,  // imageMemoryBarrierCount
		&(const vk::ImageMemoryBarrier&)vk::ImageMemoryBarrier(  // pImageMemoryBarriers
			vk::AccessFlags(),  // srcAccessMask
			vk::AccessFlagBits::eDepthStencilAttachmentRead|vk::AccessFlagBits::eDepthStencilAttachmentWrite,  // dstAccessMask
			vk::ImageLayout::eUndefined,  // oldLayout
			vk::ImageLayout::eDepthStencilAttachmentOptimal,  // newLayout
			VK_QUEUE_FAMILY_IGNORED,  // srcQueueFamilyIndex
			VK_QUEUE_FAMILY_IGNORED,  // dstQueueFamilyIndex
			depthImage.get(),  // image
			vk::ImageSubresourceRange(  // subresourceRange
				(depthFormat==vk::Format::eD32Sfloat)  // aspectMask
					?vk::ImageAspectFlagBits::eDepth
					:vk::ImageAspectFlagBits::eDepth|vk::ImageAspectFlagBits::eStencil,
				0,  // baseMipLevel
				1,  // levelCount
				0,  // baseArrayLayer
				1   // layerCount
			)
		)
	);
	cb->end();
	graphicsQueue.submit(
		vk::ArrayProxy<const vk::SubmitInfo>(
			1,
			&(const vk::SubmitInfo&)vk::SubmitInfo(
				0,nullptr,nullptr,  // waitSemaphoreCount,pWaitSemaphores,pWaitDstStageMask
				1,&cb.get(),        // commandBufferCount,pCommandBuffers
				0,nullptr           // signalSemaphoreCount,pSignalSemaphores
			)
		),
		vk::Fence(nullptr)
	);
	graphicsQueue.waitIdle();

	// pipeline
	pipeline=
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
					&(const vk::Viewport&)vk::Viewport(0.f,0.f,float(_currentSurfaceExtent.width),float(_currentSurfaceExtent.height),0.f,1.f),  // pViewports
					1,  // scissorCount
					&(const vk::Rect2D&)vk::Rect2D(vk::Offset2D(0,0),_currentSurfaceExtent)  // pScissors
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
	framebuffers.reserve(swapchainImages.size());
	for(size_t i=0,c=swapchainImages.size(); i<c; i++)
		framebuffers.emplace_back(
			device->createFramebufferUnique(
				vk::FramebufferCreateInfo(
					vk::FramebufferCreateFlags(),   // flags
					renderPass.get(),               // renderPass
					2,  // attachmentCount
					array<vk::ImageView,2>{  // pAttachments
						swapchainImageViews[i].get(),
						depthImageView.get()
					}.data(),
					_currentSurfaceExtent.width,     // width
					_currentSurfaceExtent.height,    // height
					1  // layers
				)
			)
		);

	// reallocate command buffers
	if(commandBuffers.size()!=swapchainImages.size()) {
		commandBuffers=
			device->allocateCommandBuffersUnique(
				vk::CommandBufferAllocateInfo(
					commandPool.get(),                 // commandPool
					vk::CommandBufferLevel::ePrimary,  // level
					uint32_t(swapchainImages.size())   // commandBufferCount
				)
			);
	}

	// record command buffers
	for(size_t i=0,c=swapchainImages.size(); i<c; i++) {
		vk::CommandBuffer& cb=commandBuffers[i].get();
		cb.begin(
			vk::CommandBufferBeginInfo(
				vk::CommandBufferUsageFlagBits::eSimultaneousUse,  // flags
				nullptr  // pInheritanceInfo
			)
		);
		cb.beginRenderPass(
			vk::RenderPassBeginInfo(
				renderPass.get(),       // renderPass
				framebuffers[i].get(),  // framebuffer
				vk::Rect2D(vk::Offset2D(0,0),_currentSurfaceExtent),  // renderArea
				2,                      // clearValueCount
				array<vk::ClearValue,2>{  // pClearValues
					vk::ClearColorValue(array<float,4>{0.f,0.f,0.f,1.f}),
					vk::ClearDepthStencilValue(1.f,0)
				}.data()
			),
			vk::SubpassContents::eInline
		);
		cb.bindPipeline(vk::PipelineBindPoint::eGraphics,pipeline.get());  // bind pipeline
		cb.draw(6,1,0,0);  // draw two triangles
		cb.endRenderPass();
		cb.end();
	}
}


/// Queue one frame for rendering.
void VulkanWindow::exposeEvent(QExposeEvent* e)
{
	if(!isExposed()) {
		cout<<"Unexpose"<<endl;
		return;
	}
	cout<<"Expose (size: "<<width()<<"x"<<height()<<", "<<(e->region().isEmpty()?"empty":"non-empty")<<", region: "
	    <<e->region().boundingRect().size().width()<<"x"<<e->region().boundingRect().size().height()<<")"<<endl;
	if(int(_currentSurfaceExtent.width)!=width() || int(_currentSurfaceExtent.height)!=height()) {
		cout<<"Expose not correct size."<<endl;
		requestUpdate();
		return;
	}

	// acquire next image
	uint32_t imageIndex;
	vk::Result r=device->acquireNextImageKHR(swapchain.get(),numeric_limits<uint64_t>::max(),imageAvailableSemaphore.get(),vk::Fence(nullptr),&imageIndex);
	if(r!=vk::Result::eSuccess) {
		if(r==vk::Result::eErrorOutOfDateKHR||r==vk::Result::eSuboptimalKHR)  { cout<<"Wrong size in acquireNextImageKHR()."<<endl; requestUpdate(); return; }
		//if(r==vk::Result::eErrorOutOfDateKHR||r==vk::Result::eSuboptimalKHR)  { cout<<"Wrong size in acquireNextImageKHR()."<<endl; return; }
		//if(r==vk::Result::eErrorOutOfDateKHR||r==vk::Result::eSuboptimalKHR)  throw std::runtime_error("VulkanWindow::exposeEvent(): Swapchain was not resized.");
		else  vk::throwResultException(r,VULKAN_HPP_NAMESPACE_STRING"::Device::acquireNextImageKHR");
	}

	// submit work
	graphicsQueue.submit(
		vk::ArrayProxy<const vk::SubmitInfo>(
			1,
			&(const vk::SubmitInfo&)vk::SubmitInfo(
				1,                               // waitSemaphoreCount
				&imageAvailableSemaphore.get(),  // pWaitSemaphores
				&(const vk::PipelineStageFlags&)vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput),  // pWaitDstStageMask
				1,                               // commandBufferCount
				&commandBuffers[imageIndex].get(),  // pCommandBuffers
				1,                               // signalSemaphoreCount
				&renderFinishedSemaphore.get()   // pSignalSemaphores
			)
		),
		vk::Fence(nullptr)
	);

	// submit image for presentation
	r=presentationQueue.presentKHR(
		&(const vk::PresentInfoKHR&)vk::PresentInfoKHR(
			1,                 // waitSemaphoreCount
			&renderFinishedSemaphore.get(),  // pWaitSemaphores
			1,                 // swapchainCount
			&swapchain.get(),  // pSwapchains
			&imageIndex,       // pImageIndices
			nullptr            // pResults
		)
	);
	if(r!=vk::Result::eSuccess) {
		if(r==vk::Result::eErrorOutOfDateKHR||r==vk::Result::eSuboptimalKHR)  { cout<<"Wrong size in acquireNextImageKHR()."<<endl; requestUpdate(); return; }
		//if(r==vk::Result::eErrorOutOfDateKHR)  { cout<<"eErrorOutOfDateKHR in presentKHR()."<<endl; return; }
		//if(r==vk::Result::eSuboptimalKHR)  { cout<<"eSuboptimalKHR in presentKHR()."<<endl; return; }
		//if(r==vk::Result::eErrorOutOfDateKHR||r==vk::Result::eSuboptimalKHR)  throw std::runtime_error("VulkanWindow::exposeEvent(): Swapchain was not resized.");
		else  vk::throwResultException(r,VULKAN_HPP_NAMESPACE_STRING"::Queue::presentKHR");
	}
}


bool VulkanWindow::event(QEvent* ev)
{
	// catch all exceptions as Qt5 is not exception friendly
	try {
		return inherited::event(ev);
	} catch(std::exception &e) {
		_exceptionHandler(&e);
		return true;
	} catch(...) {
		_exceptionHandler(nullptr);
		return true;
	}
}


void VulkanWindow::defaultExceptionHandler(const std::exception* e)
{
	// print error
	cout<<"VulkanWindow error: "<<(e?e->what():"unknown error")<<endl;

	// stop event loop
	QCoreApplication::exit(1);
}
