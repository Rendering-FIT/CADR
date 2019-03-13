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


// Vulkan instance
// (must be destructed as the last one, at least on Linux, it must be destroyed after display connection)
static vk::UniqueInstance instance;

// windowing variables
#ifdef _WIN32
static HWND window=nullptr;
struct Win32Cleaner {
	~Win32Cleaner() {
		if(window) {
			DestroyWindow(window);
			UnregisterClass("HelloWindow",GetModuleHandle(NULL));
		}
	}
} win32Cleaner;
#else
static Display* display=nullptr;
static Window window=0;
struct XlibCleaner {
	~XlibCleaner() {
		if(window)  XDestroyWindow(display,window);
		if(display)  XCloseDisplay(display);
	}
} xlibCleaner;
Atom wmDeleteMessage;
#endif
static vk::Extent2D currentSurfaceExtent(0,0);
static vk::Extent2D windowSize;
static bool needResize=true;

// Vulkan handles and objects
// (they need to be placed in particular (not arbitrary) order as it gives their destruction order)
static vk::UniqueSurfaceKHR surface;
static vk::PhysicalDevice physicalDevice;
static uint32_t graphicsQueueFamily;
static uint32_t presentationQueueFamily;
static vk::UniqueDevice device;
static vk::Queue graphicsQueue;
static vk::Queue presentationQueue;
static vk::SurfaceFormatKHR chosenSurfaceFormat;
static vk::UniqueRenderPass renderPass;
static vk::UniqueShaderModule vsModule;
static vk::UniqueShaderModule fsModule;
static vk::UniquePipelineCache pipelineCache;
static vk::UniquePipelineLayout pipelineLayout;
static vk::UniqueSwapchainKHR swapchain;
static vector<vk::UniqueImageView> swapchainImageViews;
static vk::UniquePipeline pipeline;
static vector<vk::UniqueFramebuffer> framebuffers;
static vk::UniqueCommandPool commandPool;
static vector<vk::UniqueCommandBuffer> commandBuffers;
static vk::UniqueSemaphore imageAvailableSemaphore;
static vk::UniqueSemaphore renderFinishedSemaphore;

// shader code in SPIR-V binary
static const uint32_t vsSpirv[]={
#include "shader.vert.spv"
};
static const uint32_t fsSpirv[]={
#include "shader.frag.spv"
};


/// Init Vulkan and open the window.
static void init()
{
	// Vulkan instance
	instance=
		vk::createInstanceUnique(
			vk::InstanceCreateInfo{
				vk::InstanceCreateFlags(),  // flags
				&(const vk::ApplicationInfo&)vk::ApplicationInfo{
					"CADR Vk HelloTriangle", // application name
					VK_MAKE_VERSION(0,0,0),  // application version
					"CADR",                  // engine name
					VK_MAKE_VERSION(0,0,0),  // engine version
					VK_API_VERSION_1_0,      // api version
				},
				0,nullptr,  // no layers
				2,          // enabled extension count
#ifdef _WIN32
				array<const char*,2>{"VK_KHR_surface","VK_KHR_win32_surface"}.data(),  // enabled extension names
#else
				array<const char*,2>{"VK_KHR_surface","VK_KHR_xlib_surface"}.data(),  // enabled extension names
#endif
			});


#ifdef _WIN32

	// initial window size
	RECT screenSize;
	if(GetWindowRect(GetDesktopWindow(),&screenSize)==0)
		throw runtime_error("GetWindowRect() failed.");
	windowSize.setWidth((screenSize.right-screenSize.left)/2);
	windowSize.setHeight((screenSize.bottom-screenSize.top)/2);

	// window's message handling procedure
	auto wndProc=[](HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)->LRESULT {
		switch(msg)
		{
			case WM_SIZE:
				needResize=true;
				windowSize.setWidth(LOWORD(lParam));
				windowSize.setHeight(HIWORD(lParam));
				return DefWindowProc(hwnd,msg,wParam,lParam);
			case WM_CLOSE:
				DestroyWindow(hwnd);
				UnregisterClass("HelloWindow",GetModuleHandle(NULL));
				window=nullptr;
				return 0;
			case WM_DESTROY:
				PostQuitMessage(0);
				return 0;
			default:
				return DefWindowProc(hwnd,msg,wParam,lParam);
		}
	};

	// register window class
	WNDCLASSEX wc;
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = 0;
	wc.lpfnWndProc   = wndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = GetModuleHandle(NULL);
	wc.hIcon         = LoadIcon(NULL,IDI_APPLICATION);
	wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "HelloWindow";
	wc.hIconSm       = LoadIcon(NULL,IDI_APPLICATION);
	if(!RegisterClassEx(&wc))
		throw runtime_error("Can not register window class.");

	// create window
	window=CreateWindowEx(
		WS_EX_CLIENTEDGE,
		"HelloWindow",
		"Hello window!",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,CW_USEDEFAULT,windowSize.width,windowSize.height,
		NULL,NULL,wc.hInstance,NULL);
	if(window==NULL) {
		UnregisterClass("HelloWindow",GetModuleHandle(NULL));
		throw runtime_error("Can not create window.");
	}

	// show window
	ShowWindow(window,SW_SHOWDEFAULT);

	// create surface
	surface=instance->createWin32SurfaceKHRUnique(vk::Win32SurfaceCreateInfoKHR(vk::Win32SurfaceCreateFlagsKHR(),wc.hInstance,window));

#else

	// open X connection
	display=XOpenDisplay(nullptr);
	if(display==nullptr)
		throw runtime_error("Can not open display. No X-server running or wrong DISPLAY variable.");

	// create window
	int blackColor=BlackPixel(display,DefaultScreen(display));
	Screen* screen=XDefaultScreenOfDisplay(display);
	windowSize.setWidth(XWidthOfScreen(screen)/2);
	windowSize.setHeight(XHeightOfScreen(screen)/2);
	window=XCreateSimpleWindow(display,DefaultRootWindow(display),0,0,windowSize.width,
	                           windowSize.height,0,blackColor,blackColor);
	XSetStandardProperties(display,window,"Hello window!","Hello window!",None,NULL,0,NULL);
	XSelectInput(display,window,StructureNotifyMask);
	wmDeleteMessage=XInternAtom(display,"WM_DELETE_WINDOW",False);
	XSetWMProtocols(display,window,&wmDeleteMessage,1);
	XMapWindow(display,window);

	// create surface
	surface=instance->createXlibSurfaceKHRUnique(vk::XlibSurfaceCreateInfoKHR(vk::XlibSurfaceCreateFlagsKHR(),display,window));

#endif

	// find compatible devices
	// (On Windows, all graphics adapters capable of monitor output are usually compatible devices.
	// On Linux X11 platform, only one graphics adapter is compatible device (the one that
	// renders the window).
	vector<vk::PhysicalDevice> deviceList=instance->enumeratePhysicalDevices();
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
		vk::createResultValue(
			pd.getSurfaceFormatsKHR(surface.get(),&formatCount,nullptr,vk::DispatchLoaderStatic()),
			VULKAN_HPP_NAMESPACE_STRING"::PhysicalDevice::getSurfaceFormatsKHR");
		uint32_t presentationModeCount;
		vk::createResultValue(
			pd.getSurfacePresentModesKHR(surface.get(),&presentationModeCount,nullptr,vk::DispatchLoaderStatic()),
			VULKAN_HPP_NAMESPACE_STRING"::PhysicalDevice::getSurfacePresentModesKHR");
		if(formatCount==0||presentationModeCount==0)
			continue;

		// select queues (for graphics rendering and for presentation)
		uint32_t graphicsQueueFamily=UINT32_MAX;
		uint32_t presentationQueueFamily=UINT32_MAX;
		vector<vk::QueueFamilyProperties> queueFamilyList=pd.getQueueFamilyProperties();
		uint32_t i=0;
		for(auto it=queueFamilyList.begin(); it!=queueFamilyList.end(); it++,i++) {
			bool p=pd.getSurfaceSupportKHR(i,surface.get())!=0;
			if(it->queueFlags&vk::QueueFlagBits::eGraphics) {
				if(p) {
					compatibleDevicesSingleQueue.emplace_back(pd,i);
					goto nextDevice;
				}
				if(graphicsQueueFamily==UINT32_MAX)
					graphicsQueueFamily=i;
			}
			else {
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
	vector<vk::SurfaceFormatKHR> surfaceFormats=physicalDevice.getSurfaceFormatsKHR(surface.get());
	const vk::SurfaceFormatKHR wantedSurfaceFormat{vk::Format::eB8G8R8A8Unorm,vk::ColorSpaceKHR::eSrgbNonlinear};
	chosenSurfaceFormat=
		surfaceFormats.size()==1&&surfaceFormats[0].format==vk::Format::eUndefined
			?wantedSurfaceFormat
			:std::find(surfaceFormats.begin(),surfaceFormats.end(),
				         wantedSurfaceFormat)!=surfaceFormats.end()
				?wantedSurfaceFormat
				:surfaceFormats[0];

	// render pass
	renderPass=
		device->createRenderPassUnique(
			vk::RenderPassCreateInfo(
				vk::RenderPassCreateFlags(),  // flags
				1,                            // attachmentCount
				&(const vk::AttachmentDescription&)vk::AttachmentDescription(  // pAttachments
					vk::AttachmentDescriptionFlags(),  // flags
					chosenSurfaceFormat.format,        // format
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


/// Recreate swapchain and pipeline. The function is usually used on each window resize event and on application start.
static void recreateSwapchainAndPipeline()
{
	// print new size
	cout<<"New window size: "<<currentSurfaceExtent.width<<"x"<<currentSurfaceExtent.height<<endl;

	// stop device and clear resources
	device->waitIdle();
	commandBuffers.clear();
	framebuffers.clear();
	pipeline.reset();
	swapchainImageViews.clear();

	// create swapchain
	vk::SurfaceCapabilitiesKHR surfaceCapabilities=physicalDevice.getSurfaceCapabilitiesKHR(surface.get());
	swapchain=
		device->createSwapchainKHRUnique(
			vk::SwapchainCreateInfoKHR(
				vk::SwapchainCreateFlagsKHR(),   // flags
				surface.get(),                   // surface
				surfaceCapabilities.maxImageCount==0  // minImageCount
					?surfaceCapabilities.minImageCount+1
					:min(surfaceCapabilities.maxImageCount,surfaceCapabilities.minImageCount+1),
				chosenSurfaceFormat.format,      // imageFormat
				chosenSurfaceFormat.colorSpace,  // imageColorSpace
				currentSurfaceExtent,  // imageExtent
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
					}(physicalDevice.getSurfacePresentModesKHR(surface.get())),
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
				&(const vk::Viewport&)vk::Viewport(0.f,0.f,float(currentSurfaceExtent.width),float(currentSurfaceExtent.height),0.f,1.f),  // pViewports
				1,  // scissorCount
				&(const vk::Rect2D&)vk::Rect2D(vk::Offset2D(0,0),currentSurfaceExtent)  // pScissors
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
	framebuffers.reserve(swapchainImages.size());
	for(size_t i=0,c=swapchainImages.size(); i<c; i++)
		framebuffers.emplace_back(
			device->createFramebufferUnique(
				vk::FramebufferCreateInfo(
					vk::FramebufferCreateFlags(),   // flags
					renderPass.get(),               // renderPass
					1,  // attachmentCount
					&swapchainImageViews[i].get(),  // pAttachments
					currentSurfaceExtent.width,     // width
					currentSurfaceExtent.height,    // height
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
				vk::Rect2D(vk::Offset2D(0,0),currentSurfaceExtent),  // renderArea
				1,                      // clearValueCount
				&(const vk::ClearValue&)vk::ClearValue(vk::ClearColorValue(array<float,4>{0.f,0.f,0.f,1.f}))  // pClearValues
			),
			vk::SubpassContents::eInline
		);
		cb.bindPipeline(vk::PipelineBindPoint::eGraphics,pipeline.get());  // bind pipeline
		cb.draw(3,1,0,0);  // draw single triangle
		cb.endRenderPass();
		cb.end();
	}
}


/// Queue one frame for rendering
static bool queueFrame()
{
	// acquire next image
	uint32_t imageIndex;
	vk::Result r=
		device->acquireNextImageKHR(
			swapchain.get(),                  // swapchain
			numeric_limits<uint64_t>::max(),  // timeout
			imageAvailableSemaphore.get(),    // semaphore to signal
			vk::Fence(nullptr),               // fence to signal
			&imageIndex                       // pImageIndex
		);
	if(r!=vk::Result::eSuccess) {
		if(r==vk::Result::eErrorOutOfDateKHR||r==vk::Result::eSuboptimalKHR) { needResize=true; return false; }
		else  vk::throwResultException(r,VULKAN_HPP_NAMESPACE_STRING"::Device::acquireNextImageKHR");
	}

	// submit work
	graphicsQueue.submit(
		vk::SubmitInfo(
			1,&imageAvailableSemaphore.get(),     // waitSemaphoreCount+pWaitSemaphores
			&(const vk::PipelineStageFlags&)vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput),  // pWaitDstStageMask
			1,&commandBuffers[imageIndex].get(),  // commandBufferCount+pCommandBuffers
			1,&renderFinishedSemaphore.get()      // signalSemaphoreCount+pSignalSemaphores
		),
		vk::Fence(nullptr)
	);

	// submit image for presentation
	r=presentationQueue.presentKHR(
		&(const vk::PresentInfoKHR&)vk::PresentInfoKHR(
			1,&renderFinishedSemaphore.get(),  // waitSemaphoreCount+pWaitSemaphores
			1,&swapchain.get(),&imageIndex,    // swapchainCount+pSwapchains+pImageIndices
			nullptr                            // pResults
		)
	);
	if(r!=vk::Result::eSuccess) {
		if(r==vk::Result::eErrorOutOfDateKHR||r==vk::Result::eSuboptimalKHR) { needResize=true; return false; }
		else  vk::throwResultException(r,VULKAN_HPP_NAMESPACE_STRING"::Queue::presentKHR");
	}

	// return success
	return true;
}


/// main function of the application
int main(int,char**)
{
	// catch exceptions
	// (vulkan.hpp fuctions throws if they fail)
	try {

		// init Vulkan and open window
		init();

#ifdef _WIN32

		// run Win32 event loop
		MSG msg;
		while(true){

			// process messages
			while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)>0) {
				if(msg.message==WM_QUIT)
					goto ExitMainLoop;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

#else

		// run Xlib event loop
		XEvent e;
		while(true) {

			// process messages
			while(XPending(display)>0) {
				XNextEvent(display,&e);
				if(e.type==ConfigureNotify && e.xconfigure.window==window) {
					vk::Extent2D newSize(e.xconfigure.width,e.xconfigure.height);
					if(newSize!=windowSize) {
						cout<<"needResize (newSize: "<<newSize.width<<"x"<<newSize.height<<", currentSize: "<<windowSize.width<<"x"<<windowSize.height<<")"<<endl;
						needResize=true;
						windowSize=newSize;
					}
					continue;
				}
				if(e.type==ClientMessage && ulong(e.xclient.data.l[0])==wmDeleteMessage)
					goto ExitMainLoop;
			}

#endif

			// recreate swapchain if necessary
			if(needResize) {

				// recreate only upon surface extent change
				vk::SurfaceCapabilitiesKHR surfaceCapabilities=physicalDevice.getSurfaceCapabilitiesKHR(surface.get());
				if(surfaceCapabilities.currentExtent!=currentSurfaceExtent) {

					// avoid 0,0 surface extent
					// by waiting for valid window size
					// (0,0 is returned on some platforms (for instance Windows) when window is minimized.
					// The creation of swapchain then raises an exception, for instance vk::Result::eErrorOutOfDeviceMemory on Windows)
					if(surfaceCapabilities.currentExtent.width==0 || surfaceCapabilities.currentExtent.height==0) {
				#ifdef _WIN32
						// stop app processing until there are messages to process
						if(GetMessage(&msg,NULL,0,0)<=0)
							goto ExitMainLoop;
						TranslateMessage(&msg);
						DispatchMessage(&msg);
				#endif
						continue;
					}

					// new currentSurfaceExtent
					currentSurfaceExtent=
						(surfaceCapabilities.currentExtent.width!=std::numeric_limits<uint32_t>::max())
							?surfaceCapabilities.currentExtent
							:vk::Extent2D{max(min(windowSize.width,surfaceCapabilities.maxImageExtent.width),surfaceCapabilities.minImageExtent.width),
										  max(min(windowSize.height,surfaceCapabilities.maxImageExtent.height),surfaceCapabilities.minImageExtent.height)};

					// recreate swapchain
					recreateSwapchainAndPipeline();
				}

				needResize=false;
			}

			// queue frame
			if(!queueFrame())
				continue;

			// wait for rendering to complete
			presentationQueue.waitIdle();
		}
	ExitMainLoop:
		device->waitIdle();

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
