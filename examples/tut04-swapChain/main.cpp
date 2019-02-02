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

#ifdef _WIN32
static HWND window=nullptr;
#else
static Display* display=nullptr;
static Window window=0;
#endif


int main(int,char**)
{
	// catch exceptions
	// (vulkan.hpp functions throws if they fail)
	try {

		// Vulkan instance
		vk::UniqueInstance instance(
			vk::createInstanceUnique(
				vk::InstanceCreateInfo{
					vk::InstanceCreateFlags(),  // flags
					&(const vk::ApplicationInfo&)vk::ApplicationInfo{
						"CADR tut04",            // application name
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
				}));


#ifdef _WIN32

		// initial window size
		RECT screenSize;
		if(GetWindowRect(GetDesktopWindow(),&screenSize)==0)
			throw runtime_error("GetWindowRect() failed.");
		uint32_t windowWidth=(screenSize.right-screenSize.left)/2;
		uint32_t windowHeight=(screenSize.bottom-screenSize.top)/2;

		// window's message handling procedure
		auto wndProc=[](HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)->LRESULT {
			switch(msg)
			{
				case WM_CLOSE:
					DestroyWindow(hwnd);
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
		wc.hbrBackground = NULL;
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = "HelloWindow";
		wc.hIconSm       = LoadIcon(NULL,IDI_APPLICATION);
		if(!RegisterClassEx(&wc))
			throw runtime_error("Can not register window class.");

		// provide destructor to clean up in the case of exception
		struct Win32Cleaner {
			~Win32Cleaner() {
				if(window)  DestroyWindow(window);
				UnregisterClass("HelloWindow",GetModuleHandle(NULL));
			}
		} win32Cleaner;

		// create window
		window=CreateWindowEx(
			WS_EX_CLIENTEDGE,
			"HelloWindow",
			"Hello window!",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,CW_USEDEFAULT,windowWidth,windowHeight,
			NULL,NULL,wc.hInstance,NULL);
		if(window==NULL)
			throw runtime_error("Can not create window.");

		// create surface
		vk::UniqueSurfaceKHR surface=
			instance->createWin32SurfaceKHRUnique(
				vk::Win32SurfaceCreateInfoKHR(vk::Win32SurfaceCreateFlagsKHR(),wc.hInstance,window)
			);

#else

		// open X connection
		// and provide destructor to clean up in the case of exception
		struct Xlib {
			Xlib() {
				display=XOpenDisplay(nullptr);
				if(display==nullptr)
					throw runtime_error("Can not open display. No X-server running or wrong DISPLAY variable.");
			}
			~Xlib() {
				if(window)  XDestroyWindow(display,window);
				if(display)  XCloseDisplay(display);
			}
		} xlib;

		// create window
		int blackColor=BlackPixel(display,DefaultScreen(display));
		Screen* screen=XDefaultScreenOfDisplay(display);
		uint32_t windowWidth=XWidthOfScreen(screen)/2;
		uint32_t windowHeight=XHeightOfScreen(screen)/2;
		window=XCreateSimpleWindow(display,DefaultRootWindow(display),0,0,windowWidth,
		                           windowHeight,0,blackColor,blackColor);
		XSetStandardProperties(display,window,"Hello window!","Hello window!",None,NULL,0,NULL);
		Atom wmDeleteMessage=XInternAtom(display,"WM_DELETE_WINDOW",False);
		XSetWMProtocols(display,window,&wmDeleteMessage,1);
		XMapWindow(display,window);

		// create surface
		vk::UniqueSurfaceKHR surface=
			instance->createXlibSurfaceKHRUnique(
				vk::XlibSurfaceCreateInfoKHR(vk::XlibSurfaceCreateFlagsKHR(),display,window)
			);

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
		vk::PhysicalDevice pd;
		uint32_t graphicsQueueFamily,presentationQueueFamily;
		if(compatibleDevicesSingleQueue.size()>0) {
			auto t=compatibleDevicesSingleQueue.front();
			pd=get<0>(t);
			graphicsQueueFamily=get<1>(t);
			presentationQueueFamily=graphicsQueueFamily;
		}
		else if(compatibleDevicesTwoQueues.size()>0) {
			auto t=compatibleDevicesTwoQueues.front();
			pd=get<0>(t);
			graphicsQueueFamily=get<1>(t);
			presentationQueueFamily=get<2>(t);
		}
		else
			throw runtime_error("No compatible devices.");
		cout<<"Using device:\n"
		      "   "<<pd.getProperties().deviceName<<endl;

		// create device
		vk::UniqueDevice device(
			pd.createDevice(
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
		vk::Queue graphicsQueue=device->getQueue(graphicsQueueFamily,0);
		vk::Queue presentationQueue=device->getQueue(presentationQueueFamily,0);

		// choose surface format
		vector<vk::SurfaceFormatKHR> surfaceFormats=pd.getSurfaceFormatsKHR(surface.get());
		const vk::SurfaceFormatKHR wantedSurfaceFormat{vk::Format::eB8G8R8A8Unorm,vk::ColorSpaceKHR::eSrgbNonlinear};
		const vk::SurfaceFormatKHR chosenSurfaceFormat=
			surfaceFormats.size()==1&&surfaceFormats[0].format==vk::Format::eUndefined
				?wantedSurfaceFormat
				:std::find(surfaceFormats.begin(),surfaceFormats.end(),
				           wantedSurfaceFormat)!=surfaceFormats.end()
					?wantedSurfaceFormat
					:surfaceFormats[0];

		// create swapchain
		vk::SurfaceCapabilitiesKHR surfaceCapabilities=pd.getSurfaceCapabilitiesKHR(surface.get());
		vk::Extent2D currentSurfaceExtent=(surfaceCapabilities.currentExtent.width!=std::numeric_limits<uint32_t>::max())
				?surfaceCapabilities.currentExtent
				:vk::Extent2D{max(min(windowWidth,surfaceCapabilities.maxImageExtent.width),surfaceCapabilities.minImageExtent.width),
				              max(min(windowHeight,surfaceCapabilities.maxImageExtent.height),surfaceCapabilities.minImageExtent.height)};
		vk::UniqueSwapchainKHR swapchain(
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
					compatibleDevicesSingleQueue.size()>0?vk::SharingMode::eExclusive:vk::SharingMode::eConcurrent,  // imageSharingMode
					compatibleDevicesSingleQueue.size()>0?uint32_t(0):uint32_t(2),  // queueFamilyIndexCount
					compatibleDevicesSingleQueue.size()>0?nullptr:array<uint32_t,2>{graphicsQueueFamily,presentationQueueFamily}.data(),  // pQueueFamilyIndices
					surfaceCapabilities.currentTransform,    // preTransform
					vk::CompositeAlphaFlagBitsKHR::eOpaque,  // compositeAlpha
					[](vector<vk::PresentModeKHR>&& modes){  // presentMode
							return find(modes.begin(),modes.end(),vk::PresentModeKHR::eMailbox)!=modes.end()
								?vk::PresentModeKHR::eMailbox
								:vk::PresentModeKHR::eFifo; // fifo is guaranteed to be supported
						}(pd.getSurfacePresentModesKHR(surface.get())),
					VK_TRUE,  // clipped
					nullptr   // oldSwapchain
				)
			)
		);

		// swapchain images and image views
		vector<vk::Image> swapchainImages=device->getSwapchainImagesKHR(swapchain.get());
		vector<vk::UniqueImageView> swapchainImageViews;
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

		// render pass
		vk::UniqueRenderPass renderPass=
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

		// framebuffers
		vector<vk::UniqueFramebuffer> framebuffers;
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

		// command pool and command buffers
		vk::UniqueCommandPool commandPool=
			device->createCommandPoolUnique(
				vk::CommandPoolCreateInfo(
					vk::CommandPoolCreateFlags(),  // flags
					graphicsQueueFamily  // queueFamilyIndex
				)
			);
		vector<vk::UniqueCommandBuffer> commandBuffers=
			device->allocateCommandBuffersUnique(
				vk::CommandBufferAllocateInfo(
					commandPool.get(),                 // commandPool
					vk::CommandBufferLevel::ePrimary,  // level
					uint32_t(swapchainImages.size())   // commandBufferCount
				)
			);

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
					&(const vk::ClearValue&)vk::ClearValue(vk::ClearColorValue(array<float,4>{0.f,1.f,0.f,1.f}))  // pClearValues
				),
				vk::SubpassContents::eInline
			);
			cb.endRenderPass();
			cb.end();
		}

		// semaphores
		vk::UniqueSemaphore imageAvailableSemaphore=
			device->createSemaphoreUnique(
				vk::SemaphoreCreateInfo(
					vk::SemaphoreCreateFlags()  // flags
				)
			);
		vk::UniqueSemaphore renderFinishedSemaphore=
			device->createSemaphoreUnique(
				vk::SemaphoreCreateInfo(
					vk::SemaphoreCreateFlags()  // flags
				)
			);


#ifdef _WIN32

		// show window
		ShowWindow(window,SW_SHOWDEFAULT);
		UpdateWindow(window);

		// run Win32 event loop
		MSG msg;
		while(true){

			// process messages
			while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			if(msg.message==WM_QUIT)
				goto ExitMainLoop;

#else

		// run Xlib event loop
		XEvent e;
		while(true) {

			// process messages
			while(XPending(display)>0) {
				XNextEvent(display,&e);
				if(e.type==ClientMessage&&ulong(e.xclient.data.l[0])==wmDeleteMessage)
					goto ExitMainLoop;
			}

#endif

			// render frame
			uint32_t imageIndex=
				device->acquireNextImageKHR(
					swapchain.get(),                  // swapchain
					numeric_limits<uint64_t>::max(),  // timeout
					imageAvailableSemaphore.get(),    // semaphore to signal
					vk::Fence(nullptr)                // fence to signal
				).value;
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
			presentationQueue.presentKHR(
				vk::PresentInfoKHR(
					1,                 // waitSemaphoreCount
					&renderFinishedSemaphore.get(),  // pWaitSemaphores
					1,                 // swapchainCount
					&swapchain.get(),  // pSwapchains
					&imageIndex,       // pImageIndices
					nullptr            // pResults
				)
			);
			presentationQueue.waitIdle();
		}

	ExitMainLoop:
		// wait for device idle
		// after main loop exit
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
