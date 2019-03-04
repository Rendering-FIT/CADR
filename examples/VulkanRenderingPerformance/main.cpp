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
static uint32_t windowWidth;
static uint32_t windowHeight;

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
static vk::Format depthFormat;
static vk::UniqueRenderPass renderPass;
static vk::UniqueShaderModule passThroughVS;
static vk::UniqueShaderModule singleUniformMatrixVS;
static vk::UniqueShaderModule fsModule;
static vk::UniquePipelineCache pipelineCache;
static vk::UniquePipelineLayout pipelineLayout;
static vk::UniquePipelineLayout singleUniformPipelineLayout;
static vk::UniqueDescriptorSetLayout singleUniformDescriptorSetLayout;
static vk::UniqueBuffer singleUniformCoherentBuffer;
static vk::UniqueBuffer singleUniformDeviceLocalBuffer;
static vk::UniqueDeviceMemory singleUniformCoherentMemory;
static vk::UniqueDeviceMemory singleUniformDeviceLocalMemory;
static vk::UniqueDescriptorPool descriptorPool;
static vk::UniqueDescriptorSet singleUniformDescriptorSet;
static vk::UniqueSwapchainKHR swapchain;
static vector<vk::UniqueImageView> swapchainImageViews;
static vk::UniqueImage depthImage;
static vk::UniqueDeviceMemory depthImageMemory;
static vk::UniqueImageView depthImageView;
static vk::UniquePipeline passThroughPipeline;
static vk::UniquePipeline singleUniformMatrixPipeline;
static vector<vk::UniqueFramebuffer> framebuffers;
static vk::UniqueCommandPool commandPool;
static vector<vk::UniqueCommandBuffer> commandBuffers;
static vk::UniqueSemaphore imageAvailableSemaphore;
static vk::UniqueSemaphore renderFinishedSemaphore;
static vk::UniqueBuffer coordinateAttribute;
static vk::UniqueDeviceMemory coordinateAttributeMemory;
static vk::UniqueQueryPool timestampPool;
static uint32_t timestampValidBits=0;
static float timestampPeriod=0;
static const size_t numTriangles=110000;
static const unsigned triangleSize=2;

// shader code in SPIR-V binary
static const uint32_t passThroughVS_spirv[]={
#include "passThrough.vert.spv"
};
static const uint32_t singleUniformMatrixVS_spirv[]={
#include "singleUniformMatrix.vert.spv"
};
static const uint32_t fsSpirv[]={
#include "shader.frag.spv"
};


static size_t getBufferSize(size_t numTriangles,bool useVec4)
{
	return numTriangles*3*(useVec4?4:3)*sizeof(float);
}


static void generate(float* vertices,size_t numTriangles,unsigned triangleSize,
                     unsigned regionWidth,unsigned regionHeight,bool useVec4,
                     double scaleX=1.,double scaleY=1.,double offsetX=0.,double offsetY=0.)
{
	unsigned stride=triangleSize+2;
	unsigned numTrianglesPerLine=regionWidth/stride*2;
	unsigned numLinesPerScreen=regionHeight/stride;
	size_t idx=0;
	size_t idxEnd=numTriangles*3*(useVec4?4:3);

	// Each iteration generates two triangles.
	// triangleSize is dimension of square that is cut into the two triangles.
	// When triangleSize is set to 1:
	//    Both triangles together produce 4 pixels: the first triangle 3 pixels and
	//    the second triangle a single pixel. For more detail, see OpenGL rasterization rules.
	for(unsigned j=0; j<numLinesPerScreen; j++) {
		for(unsigned i=0; i<numTrianglesPerLine; i+=2) {

			double x=i/2*stride;
			double y=j*stride;
			float z=0.f;

			// triangle 1, vertex 1
			vertices[idx++]=(x+0.5-0.1)*scaleX+offsetX;
			vertices[idx++]=(y+0.5-0.1)*scaleY+offsetY;
			vertices[idx++]=z;
			if(useVec4)
				vertices[idx++]=1.f;

			// triangle 1, vertex 2
			vertices[idx++]=(x+0.5+triangleSize-0.8)*scaleX+offsetX;
			vertices[idx++]=(y+0.5-0.1)*scaleY+offsetY;
			vertices[idx++]=z;
			if(useVec4)
				vertices[idx++]=1.f;

			// triangle 1, vertex 3
			vertices[idx++]=(x+0.5-0.1)*scaleX+offsetX;
			vertices[idx++]=(y+0.5+triangleSize-0.8)*scaleY+offsetY;
			vertices[idx++]=z;
			if(useVec4)
				vertices[idx++]=1.f;

			if(idx==idxEnd)
				return;

			// triangle 2, vertex 1
			vertices[idx++]=(x+0.5+triangleSize+0.6)*scaleX+offsetX;
			vertices[idx++]=(y+0.5+1.3)*scaleY+offsetY;
			vertices[idx++]=z;
			if(useVec4)
				vertices[idx++]=1.f;

			// triangle 2, vertex 2
			vertices[idx++]=(x+0.5+1.3)*scaleX+offsetX;
			vertices[idx++]=(y+0.5+triangleSize+0.6)*scaleY+offsetY;
			vertices[idx++]=z;
			if(useVec4)
				vertices[idx++]=1.f;

			// triangle 2, vertex 3
			vertices[idx++]=(x+0.5+triangleSize+0.6)*scaleX+offsetX;
			vertices[idx++]=(y+0.5+triangleSize+0.6)*scaleY+offsetY;
			vertices[idx++]=z;
			if(useVec4)
				vertices[idx++]=1.f;

			if(idx==idxEnd)
				return;
		}
	}
}


/// Init Vulkan and open the window.
static void init()
{
	// Vulkan instance
	instance=
		vk::createInstanceUnique(
			vk::InstanceCreateInfo{
				vk::InstanceCreateFlags(),  // flags
				&(const vk::ApplicationInfo&)vk::ApplicationInfo{
					"CADR Vk VulkanRenderingPerformance",  // application name
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
	windowWidth=(screenSize.right-screenSize.left)/2;
	windowHeight=(screenSize.bottom-screenSize.top)/2;

	// window's message handling procedure
	auto wndProc=[](HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)->LRESULT {
		switch(msg)
		{
			case WM_SIZE:
				windowWidth=LOWORD(lParam);
				windowHeight=HIWORD(lParam);
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
		CW_USEDEFAULT,CW_USEDEFAULT,windowWidth,windowHeight,
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
	uint32_t windowWidth=XWidthOfScreen(screen)/2;
	uint32_t windowHeight=XHeightOfScreen(screen)/2;
	window=XCreateSimpleWindow(display,DefaultRootWindow(display),0,0,windowWidth,
		                        windowHeight,0,blackColor,blackColor);
	XSetStandardProperties(display,window,"Hello window!","Hello window!",None,NULL,0,NULL);
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
		vector<bool> presentationSupport;
		presentationSupport.reserve(queueFamilyList.size());
		uint32_t i=0;
		for(auto it=queueFamilyList.begin(); it!=queueFamilyList.end(); it++,i++) {
			bool p=pd.getSurfaceSupportKHR(i,surface.get())!=0;
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
	vector<vk::SurfaceFormatKHR> surfaceFormats=physicalDevice.getSurfaceFormatsKHR(surface.get());
	const vk::SurfaceFormatKHR wantedSurfaceFormat{vk::Format::eB8G8R8A8Unorm,vk::ColorSpaceKHR::eSrgbNonlinear};
	chosenSurfaceFormat=
		surfaceFormats.size()==1&&surfaceFormats[0].format==vk::Format::eUndefined
			?wantedSurfaceFormat
			:std::find(surfaceFormats.begin(),surfaceFormats.end(),
			           wantedSurfaceFormat)!=surfaceFormats.end()
				?wantedSurfaceFormat
				:surfaceFormats[0];
	depthFormat=[](){
		for(vk::Format f:array<vk::Format,3>{vk::Format::eD32Sfloat,vk::Format::eD32SfloatS8Uint,vk::Format::eD24UnormS8Uint}) {
			vk::FormatProperties p=physicalDevice.getFormatProperties(f);
			if(p.optimalTilingFeatures&vk::FormatFeatureFlagBits::eDepthStencilAttachment)
				return f;
		}
		throw std::runtime_error("No suitable depth buffer format.");
	}();

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
	passThroughVS=device->createShaderModuleUnique(
		vk::ShaderModuleCreateInfo(
			vk::ShaderModuleCreateFlags(),  // flags
			sizeof(passThroughVS_spirv),  // codeSize
			passThroughVS_spirv  // pCode
		)
	);
	singleUniformMatrixVS=device->createShaderModuleUnique(
		vk::ShaderModuleCreateInfo(
			vk::ShaderModuleCreateFlags(),  // flags
			sizeof(singleUniformMatrixVS_spirv),  // codeSize
			singleUniformMatrixVS_spirv  // pCode
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
	singleUniformDescriptorSetLayout=
		device->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),  // flags
				1,  // bindingCount
				array<vk::DescriptorSetLayoutBinding,1>{  // pBindings
					vk::DescriptorSetLayoutBinding(
						0,  // binding
						vk::DescriptorType::eUniformBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eVertex,  // stageFlags
						nullptr  // pImmutableSamplers
					)
				}.data()
			)
		);
	singleUniformPipelineLayout=
		device->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				1,       // setLayoutCount
				&singleUniformDescriptorSetLayout.get(), // pSetLayouts
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
static bool recreateSwapchainAndPipeline()
{
	// stop device and clear resources
	device->waitIdle();
	commandBuffers.clear();
	framebuffers.clear();
	depthImage.reset();
	depthImageMemory.reset();
	depthImageView.reset();
	passThroughPipeline.reset();
	singleUniformMatrixPipeline.reset();
	swapchainImageViews.clear();

	// currentSurfaceExtent
recreateSwapchain:
	vk::SurfaceCapabilitiesKHR surfaceCapabilities=physicalDevice.getSurfaceCapabilitiesKHR(surface.get());
	vk::Extent2D currentSurfaceExtent=(surfaceCapabilities.currentExtent.width!=std::numeric_limits<uint32_t>::max())
			?surfaceCapabilities.currentExtent
			:vk::Extent2D{max(min(windowWidth,surfaceCapabilities.maxImageExtent.width),surfaceCapabilities.minImageExtent.width),
			              max(min(windowHeight,surfaceCapabilities.maxImageExtent.height),surfaceCapabilities.minImageExtent.height)};

	// avoid 0,0 surface extent
	// by waiting for valid window size
	// (0,0 is returned on some platforms (for instance Windows) when window is minimized.
	// The creation of swapchain then raises an exception, for instance vk::Result::eErrorOutOfDeviceMemory on Windows)
#ifdef _WIN32
	// run Win32 event loop
	if(currentSurfaceExtent.width==0||currentSurfaceExtent.height==0) {
		// wait for and process a message
		MSG msg;
		if(GetMessage(&msg,NULL,0,0)<=0)
			return false;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		// process remaining messages
		while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)>0) {
			if(msg.message==WM_QUIT)
				return false;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// try to recreate swapchain again
		goto recreateSwapchain;
	}
#else
	// run Xlib event loop
	if(currentSurfaceExtent.width==0||currentSurfaceExtent.height==0) {
		// process messages
		XEvent e;
		while(XPending(display)>0) {
			XNextEvent(display,&e);
			if(e.type==ClientMessage&&ulong(e.xclient.data.l[0])==wmDeleteMessage)
				return false;
		}
		// try to recreate swapchain again
		goto recreateSwapchain;
	}
#endif

	// create swapchain
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

	// depth image
	depthImage=
		device->createImageUnique(
			vk::ImageCreateInfo(
				vk::ImageCreateFlags(),  // flags
				vk::ImageType::e2D,      // imageType
				depthFormat,             // format
				vk::Extent3D(currentSurfaceExtent.width,currentSurfaceExtent.height,1),  // extent
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
			[](vk::Image depthImage)->vk::MemoryAllocateInfo{
				vk::MemoryRequirements memoryRequirements=device->getImageMemoryRequirements(depthImage);
				vk::PhysicalDeviceMemoryProperties memoryProperties=physicalDevice.getMemoryProperties();
				for(uint32_t i=0; i<memoryProperties.memoryTypeCount; i++)
					if(memoryRequirements.memoryTypeBits&(1<<i))
						if(memoryProperties.memoryTypes[i].propertyFlags&vk::MemoryPropertyFlagBits::eDeviceLocal)
							return vk::MemoryAllocateInfo(
									memoryRequirements.size,  // allocationSize
									i                         // memoryTypeIndex
								);
				throw std::runtime_error("No suitable memory type found for depth buffer.");
			}(depthImage.get())
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
	auto createPipeline=
		[](vk::ShaderModule vsModule,vk::ShaderModule fsModule,vk::PipelineLayout pipelineLayout,
		   const vk::Extent2D currentSurfaceExtent)->vk::UniquePipeline
		{
			return device->createGraphicsPipelineUnique(
				pipelineCache.get(),
				vk::GraphicsPipelineCreateInfo(
					vk::PipelineCreateFlags(),  // flags
					2,  // stageCount
					array<const vk::PipelineShaderStageCreateInfo,2>{  // pStages
						vk::PipelineShaderStageCreateInfo{
							vk::PipelineShaderStageCreateFlags(),  // flags
							vk::ShaderStageFlagBits::eVertex,      // stage
							vsModule,  // module
							"main",    // pName
							nullptr    // pSpecializationInfo
						},
						vk::PipelineShaderStageCreateInfo{
							vk::PipelineShaderStageCreateFlags(),  // flags
							vk::ShaderStageFlagBits::eFragment,    // stage
							fsModule,  // module
							"main",    // pName
							nullptr    // pSpecializationInfo
						}
					}.data(),
					&(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{  // pVertexInputState
						vk::PipelineVertexInputStateCreateFlags(),  // flags
						1,        // vertexBindingDescriptionCount
						array<const vk::VertexInputBindingDescription,1>{  // pVertexBindingDescriptions
							vk::VertexInputBindingDescription(
								0,  // binding
								4*sizeof(float),  // stride
								vk::VertexInputRate::eVertex  // inputRate
							),
						}.data(),
						1,        // vertexAttributeDescriptionCount
						array<const vk::VertexInputAttributeDescription,1>{  // pVertexAttributeDescriptions
							vk::VertexInputAttributeDescription(
								0,  // location
								0,  // binding
								vk::Format::eR32G32B32A32Sfloat,  // format
								0   // offset
							),
						}.data()
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
					pipelineLayout,  // layout
					renderPass.get(),  // renderPass
					0,  // subpass
					vk::Pipeline(nullptr),  // basePipelineHandle
					-1 // basePipelineIndex
				)
			);
		};
	passThroughPipeline=
		createPipeline(passThroughVS.get(),fsModule.get(),pipelineLayout.get(),currentSurfaceExtent);
	singleUniformMatrixPipeline=
		createPipeline(singleUniformMatrixVS.get(),fsModule.get(),singleUniformPipelineLayout.get(),currentSurfaceExtent);

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
					currentSurfaceExtent.width,     // width
					currentSurfaceExtent.height,    // height
					1  // layers
				)
			)
		);

	// vertex attribute buffers
	size_t bufferSize=getBufferSize(numTriangles,true);
	coordinateAttribute=
		device->createBufferUnique(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				bufferSize,                   // size
				vk::BufferUsageFlagBits::eVertexBuffer|vk::BufferUsageFlagBits::eTransferDst,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			)
		);

	// memory for buffers
	auto allocateMemory=
		[](vk::Buffer buffer,vk::MemoryPropertyFlags requiredFlags)->vk::UniqueDeviceMemory{
			vk::MemoryRequirements memoryRequirements=device->getBufferMemoryRequirements(buffer);
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
			throw std::runtime_error("No suitable memory type found for the buffer.");
		};

	// vertex attribute memories
	coordinateAttributeMemory=allocateMemory(coordinateAttribute.get(),vk::MemoryPropertyFlagBits::eDeviceLocal);
	device->bindBufferMemory(
		coordinateAttribute.get(),  // image
		coordinateAttributeMemory.get(),  // memory
		0  // memoryOffset
	);

	// staging buffer
	vk::UniqueBuffer coordinateStagingBuffer=
		device->createBufferUnique(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				bufferSize,                   // size
				vk::BufferUsageFlagBits::eTransferSrc,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			)
		);

	// memory for staging buffer
	vk::UniqueDeviceMemory coordinateStagingBufferMemory=
		allocateMemory(coordinateStagingBuffer.get(),
	                  vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
	device->bindBufferMemory(
		coordinateStagingBuffer.get(),  // image
		coordinateStagingBufferMemory.get(),  // memory
		0  // memoryOffset
	);

	// map coordinate staging buffer
	struct MappedMemoryDeleter {
		vk::DeviceMemory memory;
		void operator()(void*) { device->unmapMemory(memory); }
	};
	MappedMemoryDeleter mappedMemoryDeleter{coordinateStagingBufferMemory.get()};
	unique_ptr<void,MappedMemoryDeleter> mappedMemory(
		device->mapMemory(coordinateStagingBufferMemory.get(),0,VK_WHOLE_SIZE,vk::MemoryMapFlags()),  // pointer
		mappedMemoryDeleter  // deleter
	);

	// fill coordinate staging buffer
	generate(reinterpret_cast<float*>(mappedMemory.get()),numTriangles,triangleSize,1000,1000,true,
	                                  2./currentSurfaceExtent.width,2./currentSurfaceExtent.height,-1.,-1.);
	mappedMemory.reset();

	// uniform buffers and memory
	singleUniformCoherentBuffer=
		device->createBufferUnique(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				16*sizeof(float),             // size
				vk::BufferUsageFlagBits::eUniformBuffer,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			)
		);
	singleUniformDeviceLocalBuffer=
		device->createBufferUnique(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				16*sizeof(float),             // size
				vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			)
		);
	singleUniformCoherentMemory=
		allocateMemory(singleUniformCoherentBuffer.get(),
		               vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
	singleUniformDeviceLocalMemory=
		allocateMemory(singleUniformDeviceLocalBuffer.get(),
		               vk::MemoryPropertyFlagBits::eDeviceLocal);
	device->bindBufferMemory(
		singleUniformCoherentBuffer.get(),  // image
		singleUniformCoherentMemory.get(),  // memory
		0  // memoryOffset
	);
	device->bindBufferMemory(
		singleUniformDeviceLocalBuffer.get(),  // image
		singleUniformDeviceLocalMemory.get(),  // memory
		0  // memoryOffset
	);
	mappedMemoryDeleter.memory=singleUniformCoherentMemory.get();
	mappedMemory=
		unique_ptr<void,MappedMemoryDeleter>(
			device->mapMemory(singleUniformCoherentMemory.get(),0,VK_WHOLE_SIZE,vk::MemoryMapFlags()),  // pointer
			mappedMemoryDeleter  // deleter
		);

	// fill uniform with data
	constexpr float identityMatrix[]{
		1.f,0.f,0.f,0.f,
		0.f,1.f,0.f,0.f,
		0.f,0.f,1.f,0.f,
		0.f,0.f,0.f,1.f
	};
	memcpy(mappedMemory.get(),identityMatrix,sizeof(identityMatrix));
	mappedMemory.reset();

	// descriptor sets
	descriptorPool=
		device->createDescriptorPoolUnique(
			vk::DescriptorPoolCreateInfo(
				vk::DescriptorPoolCreateFlags(),  // flags
				1,  // maxSets
				1,  // poolSizeCount
				array<vk::DescriptorPoolSize,1>{  // pPoolSizes
					vk::DescriptorPoolSize(
						vk::DescriptorType::eUniformBuffer,  // type
						1  // descriptorCount
					)
				}.data()
			)
		);
	singleUniformDescriptorSet=std::move(
		device->allocateDescriptorSetsUnique(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&singleUniformDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0]);
	device->updateDescriptorSets(
		vk::WriteDescriptorSet(  // descriptorWrites
			singleUniformDescriptorSet.get(),  // dstSet
			0,  // dstBinding
			0,  // dstArrayElement
			1,  // descriptorCount
			vk::DescriptorType::eUniformBuffer,  // descriptorType
			nullptr,  // pImageInfo
			array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
				vk::DescriptorBufferInfo(
					singleUniformCoherentBuffer.get(),  // buffer
					0,  // offset
					16*sizeof(float)  // range
				),
			}.data(),
			nullptr  // pTexelBufferView
		),
		nullptr  // descriptorCopies
	);

	// timestamp properties
	timestampValidBits=
		physicalDevice.getQueueFamilyProperties()[graphicsQueueFamily].timestampValidBits;
	timestampPeriod=
		physicalDevice.getProperties().limits.timestampPeriod;
	cout<<"Timestamp number of bits: "<<timestampValidBits<<endl;
	cout<<"Timestamp period: "<<timestampPeriod<<"ns"<<endl;
	if(timestampValidBits==0)
		throw runtime_error("Timestamps are not supported.");

	// timestamp pool
	timestampPool=
		device->createQueryPoolUnique(
			vk::QueryPoolCreateInfo(
				vk::QueryPoolCreateFlags(),  // flags
				vk::QueryType::eTimestamp,   // queryType
				2,                           // queryCount
				vk::QueryPipelineStatisticFlags()  // pipelineStatistics
			)
		);

	// transient command pool
	vk::UniqueCommandPool commandPoolTransient=
		device->createCommandPoolUnique(
			vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlagBits::eTransient,  // flags
				graphicsQueueFamily  // queueFamilyIndex
			)
		);

	// allocate command buffer
	vk::UniqueCommandBuffer commandBuffer=std::move(
		device->allocateCommandBuffersUnique(
			vk::CommandBufferAllocateInfo(
				commandPoolTransient.get(),        // commandPool
				vk::CommandBufferLevel::ePrimary,  // level
				1                                  // commandBufferCount
			)
		)[0]);

	// record command buffer
	commandBuffer->begin(
		vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,  // flags
			nullptr  // pInheritanceInfo
		)
	);
	commandBuffer->copyBuffer(
		coordinateStagingBuffer.get(),  // srcBuffer
		coordinateAttribute.get(),      // dstBuffer
		1,                              // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,bufferSize)  // pRegions
	);
	commandBuffer->end();

	// submit command buffer
	vk::UniqueFence fence(device->createFenceUnique(vk::FenceCreateInfo{vk::FenceCreateFlags()}));
	graphicsQueue.submit(
		vk::SubmitInfo(  // submits (vk::ArrayProxy)
			0,nullptr,nullptr,       // waitSemaphoreCount,pWaitSemaphores,pWaitDstStageMask
			1,&commandBuffer.get(),  // commandBufferCount,pCommandBuffers
			0,nullptr                // signalSemaphoreCount,pSignalSemaphores
		),
		fence.get()  // fence
	);

	// wait for work to complete
	vk::Result r=device->waitForFences(
		fence.get(),  // fences (vk::ArrayProxy)
		VK_TRUE,      // waitAll
		3e9           // timeout (3s)
	);
	if(r==vk::Result::eTimeout)
		throw std::runtime_error("GPU timeout. Task is probably hanging.");

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
				2,                      // clearValueCount
				array<vk::ClearValue,2>{  // pClearValues
					vk::ClearColorValue(array<float,4>{0.f,0.f,0.f,1.f}),
					vk::ClearDepthStencilValue(1.f,0)
				}.data()
			),
			vk::SubpassContents::eInline
		);
#if 0
		cb.bindPipeline(vk::PipelineBindPoint::eGraphics,passThroughPipeline.get());  // bind pipeline
#else
		cb.bindPipeline(vk::PipelineBindPoint::eGraphics,singleUniformMatrixPipeline.get());  // bind pipeline
		cb.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,  // pipelineBindPoint
			singleUniformPipelineLayout.get(),  // layout
			0,  // firstSet
			singleUniformDescriptorSet.get(),  // descriptorSets
			nullptr  // dynamicOffsets
		);
#endif
		cb.bindVertexBuffers(
			0,  // firstBinding
			1,  // bindingCount
			array<const vk::Buffer,1>{  // pBuffers
				coordinateAttribute.get()
			}.data(),
			array<const vk::DeviceSize,1>{0}.data()  // pOffsets
		);
		cb.resetQueryPool(timestampPool.get(),0,2);
		cb.pipelineBarrier(
			vk::PipelineStageFlagBits::eAllCommands,  // srcStageMask
			vk::PipelineStageFlagBits::eTopOfPipe,  // dstStageMask
			vk::DependencyFlags(),  // dependencyFlags
			0,nullptr,  // memoryBarrierCount+pMemoryBarriers
			0,nullptr,  // bufferMemoryBarrierCount+pBufferMemoryBarriers
			0,nullptr   // imageMemoryBarrierCount+pImageMemoryBarriers
		);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			0  // query
		);
		cb.draw(3*numTriangles,1,0,0);  // draw two triangles
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			1  // query
		);
		cb.endRenderPass();
		cb.end();
	}

	return true;
}


/// Queue one frame for rendering
static bool queueFrame()
{
	// acquire next image
	uint32_t imageIndex;
	vk::Result r=device->acquireNextImageKHR(swapchain.get(),numeric_limits<uint64_t>::max(),imageAvailableSemaphore.get(),vk::Fence(nullptr),&imageIndex);
	if(r!=vk::Result::eSuccess) {
		if(r==vk::Result::eErrorOutOfDateKHR||r==vk::Result::eSuboptimalKHR) { if(!recreateSwapchainAndPipeline()) return false; }
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
		if(r==vk::Result::eErrorOutOfDateKHR||r==vk::Result::eSuboptimalKHR) { if(!recreateSwapchainAndPipeline()) return false; }
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

		// create swapchain and pipeline
		if(!recreateSwapchainAndPipeline())
			goto ExitMainLoop;

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
				if(e.type==ClientMessage&&ulong(e.xclient.data.l[0])==wmDeleteMessage)
					goto ExitMainLoop;
			}

#endif

			// queue frame
			if(!queueFrame())
				goto ExitMainLoop;

			// wait for rendering to complete
			presentationQueue.waitIdle();
			graphicsQueue.waitIdle();

			// read timestamps
			array<uint64_t,2> timestamps;
			device->getQueryPoolResults(
				timestampPool.get(),  // queryPool
				0,                    // firstQuery
				2,                    // queryCount
				2*sizeof(uint64_t),   // dataSize
				timestamps.data(),    // pData
				sizeof(uint64_t),     // stride
				vk::QueryResultFlagBits::e64  // flags
			);
			cout<<"Rendering time:      "<<(timestamps[1]-timestamps[0])*timestampPeriod/1e3<<"us"<<endl;
			cout<<"Triangle throughput: "<<float(numTriangles)/(float(timestamps[1]-timestamps[0])*timestampPeriod)*1e3<<" millions per second"<<endl;

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
