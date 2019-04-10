#ifdef _WIN32
# define VK_USE_PLATFORM_WIN32_KHR
#else
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# define VK_USE_PLATFORM_XLIB_KHR
#endif
#include <vulkan/vulkan.hpp>
#include <array>
#include <chrono>
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
			UnregisterClass("RenderingWindow",GetModuleHandle(NULL));
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
static vk::Format depthFormat;
static vk::UniqueRenderPass renderPass;
static vk::UniqueShaderModule attributelessConstantOutputVS;
static vk::UniqueShaderModule attributelessInputIndicesVS;
static vk::UniqueShaderModule coordinateAttributeVS;
static vk::UniqueShaderModule coordinateBufferVS;
static vk::UniqueShaderModule singleUniformMatrixVS;
static vk::UniqueShaderModule matrixAttributeVS;
static vk::UniqueShaderModule matrixBufferVS;
static vk::UniqueShaderModule constantColorFS;
static vk::UniquePipelineCache pipelineCache;
static vk::UniquePipelineLayout simplePipelineLayout;
static vk::UniquePipelineLayout singleUniformPipelineLayout;
static vk::UniquePipelineLayout coordinateBufferPipelineLayout;
static vk::UniquePipelineLayout matrixBufferPipelineLayout;
static vk::UniqueDescriptorSetLayout singleUniformDescriptorSetLayout;
static vk::UniqueDescriptorSetLayout coordinateBufferDescriptorSetLayout;
static vk::UniqueDescriptorSetLayout matrixBufferDescriptorSetLayout;
static vk::UniqueBuffer singleUniformBuffer;
static vk::UniqueBuffer sameMatrixBuffer;
static vk::UniqueBuffer transformationMatrixBuffer;
static vk::UniqueBuffer indirectBuffer;
static vk::UniqueDeviceMemory singleUniformMemory;
static vk::UniqueDeviceMemory sameMatrixMemory;
static vk::UniqueDeviceMemory transformationMatrixMemory;
static vk::UniqueDeviceMemory indirectBufferMemory;
static vk::UniqueDescriptorPool singleUniformDescriptorPool;
static vk::UniqueDescriptorPool coordinateBufferDescriptorPool;
static vk::UniqueDescriptorPool sameMatrixBufferDescriptorPool;
static vk::UniqueDescriptorPool transformationMatrixBufferDescriptorPool;
static vk::UniqueDescriptorSet singleUniformDescriptorSet;
static vk::UniqueDescriptorSet coordinateBufferDescriptorSet;
static vk::UniqueDescriptorSet sameMatrixBufferDescriptorSet;
static vk::UniqueDescriptorSet transformationMatrixBufferDescriptorSet;
static vk::UniqueSwapchainKHR swapchain;
static vector<vk::UniqueImageView> swapchainImageViews;
static vk::UniqueImage depthImage;
static vk::UniqueDeviceMemory depthImageMemory;
static vk::UniqueImageView depthImageView;
static vk::UniquePipeline attributelessConstantOutputPipeline;
static vk::UniquePipeline attributelessInputIndicesPipeline;
static vk::UniquePipeline coordinateAttributePipeline;
static vk::UniquePipeline coordinateBufferPipeline;
static vk::UniquePipeline singleUniformMatrixPipeline;
static vk::UniquePipeline matrixAttributePipeline;
static vk::UniquePipeline matrixBufferPipeline;
static vector<vk::UniqueFramebuffer> framebuffers;
static vk::UniqueCommandPool commandPool;
static vector<vk::UniqueCommandBuffer> commandBuffers;
static vk::UniqueSemaphore imageAvailableSemaphore;
static vk::UniqueSemaphore renderFinishedSemaphore;
static vk::UniqueBuffer coordinateAttribute;
static vk::UniqueDeviceMemory coordinateAttributeMemory;
static vk::UniqueQueryPool timestampPool;
static uint32_t timestampValidBits=0;
static float timestampPeriod_ns=0;
static const size_t numTriangles=size_t(1*1e6);
static const unsigned triangleSize=0;

// shader code in SPIR-V binary
static const uint32_t attributelessConstantOutputVS_spirv[]={
#include "attributelessConstantOutput.vert.spv"
};
static const uint32_t attributelessInputIndicesVS_spirv[]={
#include "attributelessInputIndices.vert.spv"
};
static const uint32_t coordinateAttributeVS_spirv[]={
#include "coordinateAttribute.vert.spv"
};
static const uint32_t coordinateBufferVS_spirv[]={
#include "coordinateBuffer.vert.spv"
};
static const uint32_t singleUniformMatrixVS_spirv[]={
#include "singleUniformMatrix.vert.spv"
};
static const uint32_t matrixAttributeVS_spirv[]={
#include "matrixAttribute.vert.spv"
};
static const uint32_t matrixBufferVS_spirv[]={
#include "matrixBuffer.vert.spv"
};
static const uint32_t constantColorFS_spirv[]={
#include "constantColor.frag.spv"
};

struct Test {
	vector<uint64_t> renderingTimes;
	string resultString;
	Test(const char* resultString_) : resultString(resultString_)  {}
};
static vector<Test> tests={
	Test("One draw call, attributeless, constant output"),
	Test("One draw call, attributeless, input indices"),
	Test("One draw call, attributeless instancing"),
	Test("One draw call, coordinates in attribute"),
	Test("One draw call, coordinates in buffer"),
	Test("One draw call, one uniform transformation"),
	Test("One draw call, per-triangle transformation in attribute"),
	Test("One draw call, per-triangle transformation in buffer"),
	Test("Per-triangle draw call, no transformations"),
	Test("Indirect buffer instancing, attributeless"),
	Test("Indirect buffer per-triangle record, attributeless"),
	Test("Indirect buffer per-triangle record"),
};


static size_t getBufferSize(size_t numTriangles,bool useVec4)
{
	return numTriangles*3*(useVec4?4:3)*sizeof(float);
}


static void generateCoordinates(float* vertices,size_t numTriangles,unsigned triangleSize,
                                unsigned regionWidth,unsigned regionHeight,bool useVec4,
                                double scaleX=1.,double scaleY=1.,double offsetX=0.,double offsetY=0.)
{
	unsigned stride=triangleSize+2;
	unsigned numTrianglesPerLine=regionWidth/stride*2;
	unsigned numLinesPerScreen=regionHeight/stride;
	size_t idx=0;
	size_t idxEnd=numTriangles*(useVec4?4:3)*3;

	// Each iteration generates two triangles.
	// triangleSize is dimension of square that is cut into the two triangles.
	// When triangleSize is set to 1:
	//    Both triangles together produce 4 pixels: the first triangle 3 pixels and
	//    the second triangle a single pixel. For more detail, see OpenGL rasterization rules.
	for(float z=0.9f; z>0.01f; z-=0.01f) {
		for(unsigned j=0; j<numLinesPerScreen; j++) {
			for(unsigned i=0; i<numTrianglesPerLine; i+=2) {

				double x=i/2*stride;
				double y=j*stride;

				// triangle 1, vertex 1
				vertices[idx++]=float((x+0.5-0.1)*scaleX+offsetX);
				vertices[idx++]=float((y+0.5-0.1)*scaleY+offsetY);
				vertices[idx++]=z;
				if(useVec4)
					vertices[idx++]=1.f;

				// triangle 1, vertex 2
				vertices[idx++]=float((x+0.5+triangleSize-0.8)*scaleX+offsetX);
				vertices[idx++]=float((y+0.5-0.1)*scaleY+offsetY);
				vertices[idx++]=z;
				if(useVec4)
					vertices[idx++]=1.f;

				// triangle 1, vertex 3
				vertices[idx++]=float((x+0.5-0.1)*scaleX+offsetX);
				vertices[idx++]=float((y+0.5+triangleSize-0.8)*scaleY+offsetY);
				vertices[idx++]=z;
				if(useVec4)
					vertices[idx++]=1.f;

				if(idx==idxEnd)
					return;

				// triangle 2, vertex 1
				vertices[idx++]=float((x+0.5+triangleSize+0.6)*scaleX+offsetX);
				vertices[idx++]=float((y+0.5+1.3)*scaleY+offsetY);
				vertices[idx++]=z;
				if(useVec4)
					vertices[idx++]=1.f;

				// triangle 2, vertex 2
				vertices[idx++]=float((x+0.5+1.3)*scaleX+offsetX);
				vertices[idx++]=float((y+0.5+triangleSize+0.6)*scaleY+offsetY);
				vertices[idx++]=z;
				if(useVec4)
					vertices[idx++]=1.f;

				// triangle 2, vertex 3
				vertices[idx++]=float((x+0.5+triangleSize+0.6)*scaleX+offsetX);
				vertices[idx++]=float((y+0.5+triangleSize+0.6)*scaleY+offsetY);
				vertices[idx++]=z;
				if(useVec4)
					vertices[idx++]=1.f;

				if(idx==idxEnd)
					return;
			}
		}
	}

	// throw if we did not managed to put all the triangles in designed area
	throw std::runtime_error("Triangles do not fit into the rendered area.");
}


static void generateMatrices(float* matrices,size_t numMatrices,unsigned triangleSize,
                             unsigned regionWidth,unsigned regionHeight,
                             double scaleX=1.,double scaleY=1.,double offsetX=0.,double offsetY=0.)
{
	unsigned stride=triangleSize+2;
	unsigned numTrianglesPerLine=regionWidth/stride;
	unsigned numLinesPerScreen=regionHeight/stride;
	size_t idx=0;
	size_t idxEnd=numMatrices*16;

	// Each iteration generates two triangles.
	// triangleSize is dimension of square that is cut into the two triangles.
	// When triangleSize is set to 1:
	//    Both triangles together produce 4 pixels: the first triangle 3 pixels and
	//    the second triangle a single pixel. For more detail, see OpenGL rasterization rules.
	for(float z=0.9f; z>0.01f; z-=0.01f) {
		for(unsigned j=0; j<numLinesPerScreen; j++) {
			for(unsigned i=0; i<numTrianglesPerLine; i++) {

				double x=i*stride;
				double y=j*stride;

				float m[]{
					1.f,0.f,0.f,0.f,
					0.f,1.f,0.f,0.f,
					0.f,0.f,1.f,0.f,
					float(x*scaleX+offsetX), float(y*scaleY+offsetY), z-0.9f, 1.f
				};
				memcpy(&matrices[idx],m,sizeof(m));
				idx+=16;

				if(idx==idxEnd)
					return;

			}
		}
	}

	// throw if we did not managed to put all the triangles in designed area
	throw std::runtime_error("Triangles do not fit into the rendered area.");
}


// allocate memory for buffers
static vk::UniqueDeviceMemory allocateMemory(vk::Buffer buffer,vk::MemoryPropertyFlags requiredFlags)
{
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
}


/// Init Vulkan and open the window.
static void init(size_t deviceIndex)
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
	windowSize.setWidth(screenSize.right-screenSize.left);
	windowSize.setHeight(screenSize.bottom-screenSize.top);

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
				UnregisterClass("RenderingWindow",GetModuleHandle(NULL));
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
	wc.lpszClassName = "RenderingWindow";
	wc.hIconSm       = LoadIcon(NULL,IDI_APPLICATION);
	if(!RegisterClassEx(&wc))
		throw runtime_error("Can not register window class.");

	// create window
	window=CreateWindowEx(
		WS_EX_CLIENTEDGE,
		"RenderingWindow",
		"Rendering performance",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,CW_USEDEFAULT,windowSize.width,windowSize.height,
		NULL,NULL,wc.hInstance,NULL);
	if(window==NULL) {
		UnregisterClass("RenderingWindow",GetModuleHandle(NULL));
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
	windowSize.setWidth(XWidthOfScreen(screen));
	windowSize.setHeight(XHeightOfScreen(screen));
	window=XCreateSimpleWindow(display,DefaultRootWindow(display),0,0,windowSize.width,
	                           windowSize.height,0,blackColor,blackColor);
	XSetStandardProperties(display,window,"Rendering performance",NULL,None,NULL,0,NULL);
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
	if(deviceIndex<compatibleDevicesSingleQueue.size()) {
		auto t=compatibleDevicesSingleQueue[deviceIndex];
		physicalDevice=get<0>(t);
		graphicsQueueFamily=get<1>(t);
		presentationQueueFamily=graphicsQueueFamily;
	}
	else if((deviceIndex-compatibleDevicesSingleQueue.size())<compatibleDevicesTwoQueues.size()) {
		auto t=compatibleDevicesTwoQueues[deviceIndex-compatibleDevicesSingleQueue.size()];
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
	depthFormat=
		[](){
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
					0,                     // srcSubpass
					0,                     // dstSubpass
					vk::PipelineStageFlagBits::eAllCommands,  // srcStageMask
					vk::PipelineStageFlagBits::eColorAttachmentOutput|vk::PipelineStageFlagBits::eTopOfPipe,  // dstStageMask
					vk::AccessFlags(),     // srcAccessMask
					vk::AccessFlagBits::eColorAttachmentRead|vk::AccessFlagBits::eColorAttachmentWrite,  // dstAccessMask
					vk::DependencyFlags()  // dependencyFlags
				)
			)
		);

	// create shader modules
	attributelessConstantOutputVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(attributelessConstantOutputVS_spirv),  // codeSize
				attributelessConstantOutputVS_spirv           // pCode
			)
		);
	attributelessInputIndicesVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(attributelessInputIndicesVS_spirv),  // codeSize
				attributelessInputIndicesVS_spirv           // pCode
			)
		);
	coordinateAttributeVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),        // flags
				sizeof(coordinateAttributeVS_spirv),  // codeSize
				coordinateAttributeVS_spirv           // pCode
			)
		);
	coordinateBufferVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),     // flags
				sizeof(coordinateBufferVS_spirv),  // codeSize
				coordinateBufferVS_spirv           // pCode
			)
		);
	singleUniformMatrixVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),        // flags
				sizeof(singleUniformMatrixVS_spirv),  // codeSize
				singleUniformMatrixVS_spirv           // pCode
			)
		);
	matrixAttributeVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),    // flags
				sizeof(matrixAttributeVS_spirv),  // codeSize
				matrixAttributeVS_spirv           // pCode
			)
		);
	matrixBufferVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(matrixBufferVS_spirv),   // codeSize
				matrixBufferVS_spirv            // pCode
			)
		);
	constantColorFS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(constantColorFS_spirv),  // codeSize
				constantColorFS_spirv           // pCode
			)
		);

	// pipeline cache
	pipelineCache=
		device->createPipelineCacheUnique(
			vk::PipelineCacheCreateInfo(
				vk::PipelineCacheCreateFlags(),  // flags
				0,       // initialDataSize
				nullptr  // pInitialData
			)
		);

	// descriptor set layouts
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
	coordinateBufferDescriptorSetLayout=
		device->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),  // flags
				1,  // bindingCount
				array<vk::DescriptorSetLayoutBinding,1>{  // pBindings
					vk::DescriptorSetLayoutBinding(
						0,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eVertex,  // stageFlags
						nullptr  // pImmutableSamplers
					)
				}.data()
			)
		);
	matrixBufferDescriptorSetLayout=
		device->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),  // flags
				1,  // bindingCount
				array<vk::DescriptorSetLayoutBinding,1>{  // pBindings
					vk::DescriptorSetLayoutBinding(
						0,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eVertex,  // stageFlags
						nullptr  // pImmutableSamplers
					)
				}.data()
			)
		);

	// pipeline layouts
	simplePipelineLayout=
		device->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				0,       // setLayoutCount
				nullptr, // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			}
		);
	singleUniformPipelineLayout=
		device->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				1,       // setLayoutCount
				&singleUniformDescriptorSetLayout.get(),  // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			}
		);
	coordinateBufferPipelineLayout=
		device->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				1,       // setLayoutCount
				&coordinateBufferDescriptorSetLayout.get(),  // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			}
		);
	matrixBufferPipelineLayout=
		device->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				1,       // setLayoutCount
				&matrixBufferDescriptorSetLayout.get(),  // pSetLayouts
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
	depthImage.reset();
	depthImageMemory.reset();
	depthImageView.reset();
	coordinateAttributePipeline.reset();
	coordinateBufferPipeline.reset();
	singleUniformMatrixPipeline.reset();
	matrixAttributePipeline.reset();
	matrixBufferPipeline.reset();
	attributelessConstantOutputPipeline.reset();
	attributelessInputIndicesPipeline.reset();
	swapchainImageViews.clear();
	coordinateAttribute.reset();
	coordinateAttributeMemory.reset();
	singleUniformBuffer.reset();
	singleUniformMemory.reset();
	sameMatrixBuffer.reset();
	sameMatrixMemory.reset();
	transformationMatrixBuffer.reset();
	transformationMatrixMemory.reset();
	singleUniformDescriptorSet.reset();
	coordinateBufferDescriptorSet.reset();
	sameMatrixBufferDescriptorSet.reset();
	transformationMatrixBufferDescriptorSet.reset();
	singleUniformDescriptorPool.reset();
	coordinateBufferDescriptorPool.reset();
	sameMatrixBufferDescriptorPool.reset();
	transformationMatrixBufferDescriptorPool.reset();
	timestampPool.reset();

	// submitNowCommandBuffer
	// that will be submitted at the end of this function
	vk::UniqueCommandPool commandPoolTransient=
		device->createCommandPoolUnique(
			vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlagBits::eTransient,  // flags
				graphicsQueueFamily  // queueFamilyIndex
			)
		);
	vk::UniqueCommandBuffer submitNowCommandBuffer=std::move(
		device->allocateCommandBuffersUnique(
			vk::CommandBufferAllocateInfo(
				commandPoolTransient.get(),        // commandPool
				vk::CommandBufferLevel::ePrimary,  // level
				1                                  // commandBufferCount
			)
		)[0]);
	submitNowCommandBuffer->begin(
		vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,  // flags
			nullptr  // pInheritanceInfo
		)
	);

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
	submitNowCommandBuffer->pipelineBarrier(
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

	// pipeline
	auto createPipeline=
		[](vk::ShaderModule vsModule,vk::ShaderModule fsModule,vk::PipelineLayout pipelineLayout,
		   const vk::Extent2D currentSurfaceExtent,
		   const vk::PipelineVertexInputStateCreateInfo* vertexInputState=nullptr)->vk::UniquePipeline
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
					vertexInputState!=nullptr  // pVertexInputState
						?vertexInputState
						:&(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
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
	attributelessConstantOutputPipeline=
		createPipeline(attributelessConstantOutputVS.get(),constantColorFS.get(),simplePipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               });
	attributelessInputIndicesPipeline=
		createPipeline(attributelessInputIndicesVS.get(),constantColorFS.get(),simplePipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               });
	coordinateAttributePipeline=
		createPipeline(coordinateAttributeVS.get(),constantColorFS.get(),simplePipelineLayout.get(),currentSurfaceExtent);
	coordinateBufferPipeline=
		createPipeline(coordinateBufferVS.get(),constantColorFS.get(),coordinateBufferPipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               });
	singleUniformMatrixPipeline=
		createPipeline(singleUniformMatrixVS.get(),constantColorFS.get(),singleUniformPipelineLayout.get(),currentSurfaceExtent);
	matrixAttributePipeline=
		createPipeline(matrixAttributeVS.get(),constantColorFS.get(),simplePipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               2,  // vertexBindingDescriptionCount
			               array<const vk::VertexInputBindingDescription,2>{  // pVertexBindingDescriptions
				               vk::VertexInputBindingDescription(
					               0,  // binding
					               4*sizeof(float),  // stride
					               vk::VertexInputRate::eVertex  // inputRate
				               ),
				               vk::VertexInputBindingDescription(
					               1,  // binding
					               16*sizeof(float),  // stride
					               vk::VertexInputRate::eInstance  // inputRate
				               ),
			               }.data(),
			               5,  // vertexAttributeDescriptionCount
			               array<const vk::VertexInputAttributeDescription,5>{  // pVertexAttributeDescriptions
				               vk::VertexInputAttributeDescription(
					               0,  // location
					               0,  // binding
					               vk::Format::eR32G32B32A32Sfloat,  // format
					               0   // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               1,  // location
					               1,  // binding
					               vk::Format::eR32G32B32A32Sfloat,  // format
					               0   // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               2,  // location
					               1,  // binding
					               vk::Format::eR32G32B32A32Sfloat,  // format
					               4*sizeof(float)  // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               3,  // location
					               1,  // binding
					               vk::Format::eR32G32B32A32Sfloat,  // format
					               8*sizeof(float)  // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               4,  // location
					               1,  // binding
					               vk::Format::eR32G32B32A32Sfloat,  // format
					               12*sizeof(float)  // offset
				               ),
			               }.data()
		               });
	matrixBufferPipeline=
		createPipeline(matrixBufferVS.get(),constantColorFS.get(),matrixBufferPipelineLayout.get(),currentSurfaceExtent);

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
	size_t coordinateBufferSize=getBufferSize(numTriangles,true);
	coordinateAttribute=
		device->createBufferUnique(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				coordinateBufferSize,         // size
				vk::BufferUsageFlagBits::eVertexBuffer|vk::BufferUsageFlagBits::eTransferDst,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			)
		);

	// vertex attribute memory
	coordinateAttributeMemory=allocateMemory(coordinateAttribute.get(),vk::MemoryPropertyFlagBits::eDeviceLocal);
	device->bindBufferMemory(
		coordinateAttribute.get(),  // image
		coordinateAttributeMemory.get(),  // memory
		0  // memoryOffset
	);


	// staging buffer struct
	struct StagingBuffer {
		vk::UniqueBuffer buffer;
		vk::UniqueDeviceMemory memory;
		void* ptr = nullptr;
		void* map()  { if(!ptr) ptr=device->mapMemory(memory.get(),0,VK_WHOLE_SIZE,vk::MemoryMapFlags()); return ptr; }
		void unmap()  { if(ptr){ device->unmapMemory(memory.get()); ptr=nullptr; } }
		~StagingBuffer() { unmap(); }
		StagingBuffer() = default;
		StagingBuffer(StagingBuffer&& s) = default;
		StagingBuffer(size_t bufferSize)
		{
			buffer=
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
			memory=
				allocateMemory(buffer.get(),
									vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
			device->bindBufferMemory(
				buffer.get(),  // image
				memory.get(),  // memory
				0  // memoryOffset
			);
		}
	};

	// coordinate staging buffer
	StagingBuffer coordinateStagingBuffer(coordinateBufferSize);
	generateCoordinates(
		reinterpret_cast<float*>(coordinateStagingBuffer.map()),numTriangles,triangleSize,
		1000,1000,true,2./currentSurfaceExtent.width,2./currentSurfaceExtent.height,-1.,-1.);
	coordinateStagingBuffer.unmap();

	// copy data from staging to attribute buffer
	submitNowCommandBuffer->copyBuffer(
		coordinateStagingBuffer.buffer.get(),  // srcBuffer
		coordinateAttribute.get(),             // dstBuffer
		1,                                     // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,coordinateBufferSize)  // pRegions
	);

	// uniform buffers and memory
	singleUniformBuffer=
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
	singleUniformMemory=
		allocateMemory(singleUniformBuffer.get(),
		               vk::MemoryPropertyFlagBits::eDeviceLocal);
	device->bindBufferMemory(
		singleUniformBuffer.get(),  // image
		singleUniformMemory.get(),  // memory
		0  // memoryOffset
	);

	// single uniform staging buffer
	constexpr float singleUniformMatrix[]{
		1.f,0.f,0.f,0.f,
		0.f,1.f,0.f,0.f,
		0.f,0.f,1.f,0.f,
		0.125f,0.f,0.f,1.f
	};
	StagingBuffer singleUniformStagingBuffer(sizeof(singleUniformMatrix));
	memcpy(singleUniformStagingBuffer.map(),singleUniformMatrix,sizeof(singleUniformMatrix));
	singleUniformStagingBuffer.unmap();

	// copy data from staging to uniform buffer
	submitNowCommandBuffer->copyBuffer(
		singleUniformStagingBuffer.buffer.get(),  // srcBuffer
		singleUniformBuffer.get(),                // dstBuffer
		1,                                        // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,sizeof(singleUniformMatrix))  // pRegions
	);

	// same matrix buffer
	sameMatrixBuffer=
		device->createBufferUnique(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				numTriangles*16*sizeof(float),  // size
				vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			)
		);
	sameMatrixMemory=
		allocateMemory(sameMatrixBuffer.get(),
		               vk::MemoryPropertyFlagBits::eDeviceLocal);
	device->bindBufferMemory(
		sameMatrixBuffer.get(),  // image
		sameMatrixMemory.get(),  // memory
		0  // memoryOffset
	);

	// same matrix staging buffer
	constexpr float matrixData[]{
		1.f,0.f,0.f,0.f,
		0.f,1.f,0.f,0.f,
		0.f,0.f,1.f,0.f,
		0.f,0.0625f,0.f,1.f
	};
	StagingBuffer sameMatrixStagingBuffer(numTriangles*sizeof(matrixData));
	sameMatrixStagingBuffer.map();
	for(size_t i=0; i<numTriangles; i++)
		memcpy(reinterpret_cast<char*>(sameMatrixStagingBuffer.ptr)+(i*sizeof(matrixData)),matrixData,sizeof(matrixData));
	sameMatrixStagingBuffer.unmap();

	// copy data from staging to uniform buffer
	submitNowCommandBuffer->copyBuffer(
		sameMatrixStagingBuffer.buffer.get(),  // srcBuffer
		sameMatrixBuffer.get(),                // dstBuffer
		1,                                     // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,numTriangles*sizeof(matrixData))  // pRegions
	);

	// transformation matrix buffer
	transformationMatrixBuffer=
		device->createBufferUnique(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				numTriangles*16*sizeof(float),  // size
				vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			)
		);
	transformationMatrixMemory=
		allocateMemory(transformationMatrixBuffer.get(),
		               vk::MemoryPropertyFlagBits::eDeviceLocal);
	device->bindBufferMemory(
		transformationMatrixBuffer.get(),  // image
		transformationMatrixMemory.get(),  // memory
		0  // memoryOffset
	);

	// same matrix staging buffer
	StagingBuffer transformationMatrixStagingBuffer(numTriangles*16*sizeof(float));
	generateMatrices(
		reinterpret_cast<float*>(transformationMatrixStagingBuffer.map()),numTriangles,triangleSize,
		1000,1000,2./currentSurfaceExtent.width,2./currentSurfaceExtent.height,0.,0.);
	transformationMatrixStagingBuffer.unmap();

	// copy data from staging to uniform buffer
	submitNowCommandBuffer->copyBuffer(
		transformationMatrixStagingBuffer.buffer.get(),  // srcBuffer
		transformationMatrixBuffer.get(),                // dstBuffer
		1,                                               // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,numTriangles*16*sizeof(float))  // pRegions
	);

	// indirect buffer
	indirectBuffer=
		device->createBufferUnique(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				(numTriangles+1)*sizeof(vk::DrawIndirectCommand),  // size
				vk::BufferUsageFlagBits::eIndirectBuffer|vk::BufferUsageFlagBits::eTransferDst,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			)
		);
	indirectBufferMemory=
		allocateMemory(indirectBuffer.get(),
		               vk::MemoryPropertyFlagBits::eDeviceLocal);
	device->bindBufferMemory(
		indirectBuffer.get(),  // image
		indirectBufferMemory.get(),  // memory
		0  // memoryOffset
	);

	// indirect staging buffer
	StagingBuffer indirectStagingBuffer((numTriangles+1)*sizeof(vk::DrawIndirectCommand));
	auto indirectBufferPtr=reinterpret_cast<vk::DrawIndirectCommand*>(indirectStagingBuffer.map());
	for(size_t i=0; i<numTriangles; i++) {
		indirectBufferPtr[i].vertexCount=3;
		indirectBufferPtr[i].instanceCount=1;
		indirectBufferPtr[i].firstVertex=uint32_t(i)*3;
		indirectBufferPtr[i].firstInstance=0;
	}
	indirectBufferPtr[numTriangles].vertexCount=3;
	indirectBufferPtr[numTriangles].instanceCount=numTriangles;
	indirectBufferPtr[numTriangles].firstVertex=0;
	indirectBufferPtr[numTriangles].firstInstance=0;
	indirectStagingBuffer.unmap();

	// copy data from staging to uniform buffer
	submitNowCommandBuffer->copyBuffer(
		indirectStagingBuffer.buffer.get(),  // srcBuffer
		indirectBuffer.get(),                // dstBuffer
		1,                                   // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,(numTriangles+1)*sizeof(vk::DrawIndirectCommand))  // pRegions
	);

	// submit command buffer
	submitNowCommandBuffer->end();
	vk::UniqueFence fence(device->createFenceUnique(vk::FenceCreateInfo{vk::FenceCreateFlags()}));
	graphicsQueue.submit(
		vk::SubmitInfo(  // submits (vk::ArrayProxy)
			0,nullptr,nullptr,       // waitSemaphoreCount,pWaitSemaphores,pWaitDstStageMask
			1,&submitNowCommandBuffer.get(),  // commandBufferCount,pCommandBuffers
			0,nullptr                // signalSemaphoreCount,pSignalSemaphores
		),
		fence.get()  // fence
	);

	// wait for work to complete
	vk::Result r=device->waitForFences(
		fence.get(),   // fences (vk::ArrayProxy)
		VK_TRUE,       // waitAll
		uint64_t(3e9)  // timeout (3s)
	);
	if(r==vk::Result::eTimeout)
		throw std::runtime_error("GPU timeout. Task is probably hanging.");


	// descriptor sets
	singleUniformDescriptorPool=
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
	coordinateBufferDescriptorPool=
		device->createDescriptorPoolUnique(
			vk::DescriptorPoolCreateInfo(
				vk::DescriptorPoolCreateFlags(),  // flags
				1,  // maxSets
				1,  // poolSizeCount
				array<vk::DescriptorPoolSize,1>{  // pPoolSizes
					vk::DescriptorPoolSize(
						vk::DescriptorType::eStorageBuffer,  // type
						1  // descriptorCount
					)
				}.data()
			)
		);
	sameMatrixBufferDescriptorPool=
		device->createDescriptorPoolUnique(
			vk::DescriptorPoolCreateInfo(
				vk::DescriptorPoolCreateFlags(),  // flags
				1,  // maxSets
				1,  // poolSizeCount
				array<vk::DescriptorPoolSize,1>{  // pPoolSizes
					vk::DescriptorPoolSize(
						vk::DescriptorType::eStorageBuffer,  // type
						1  // descriptorCount
					)
				}.data()
			)
		);
	transformationMatrixBufferDescriptorPool=
		device->createDescriptorPoolUnique(
			vk::DescriptorPoolCreateInfo(
				vk::DescriptorPoolCreateFlags(),  // flags
				1,  // maxSets
				1,  // poolSizeCount
				array<vk::DescriptorPoolSize,1>{  // pPoolSizes
					vk::DescriptorPoolSize(
						vk::DescriptorType::eStorageBuffer,  // type
						1  // descriptorCount
					)
				}.data()
			)
		);
	singleUniformDescriptorSet=std::move(
		device->allocateDescriptorSetsUnique(
			vk::DescriptorSetAllocateInfo(
				singleUniformDescriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&singleUniformDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0]);
	coordinateBufferDescriptorSet=std::move(
		device->allocateDescriptorSetsUnique(
			vk::DescriptorSetAllocateInfo(
				coordinateBufferDescriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&coordinateBufferDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0]);
	sameMatrixBufferDescriptorSet=std::move(
		device->allocateDescriptorSetsUnique(
			vk::DescriptorSetAllocateInfo(
				sameMatrixBufferDescriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&matrixBufferDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0]);
	transformationMatrixBufferDescriptorSet=std::move(
		device->allocateDescriptorSetsUnique(
			vk::DescriptorSetAllocateInfo(
				transformationMatrixBufferDescriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&matrixBufferDescriptorSetLayout.get()  // pSetLayouts
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
					singleUniformBuffer.get(),  // buffer
					0,  // offset
					16*sizeof(float)  // range
				),
			}.data(),
			nullptr  // pTexelBufferView
		),
		nullptr  // descriptorCopies
	);
	device->updateDescriptorSets(
		vk::WriteDescriptorSet(  // descriptorWrites
			coordinateBufferDescriptorSet.get(),  // dstSet
			0,  // dstBinding
			0,  // dstArrayElement
			1,  // descriptorCount
			vk::DescriptorType::eStorageBuffer,  // descriptorType
			nullptr,  // pImageInfo
			array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
				vk::DescriptorBufferInfo(
					coordinateAttribute.get(),  // buffer
					0,  // offset
					coordinateBufferSize  // range
				),
			}.data(),
			nullptr  // pTexelBufferView
		),
		nullptr  // descriptorCopies
	);
	device->updateDescriptorSets(
		vk::WriteDescriptorSet(  // descriptorWrites
			sameMatrixBufferDescriptorSet.get(),  // dstSet
			0,  // dstBinding
			0,  // dstArrayElement
			1,  // descriptorCount
			vk::DescriptorType::eStorageBuffer,  // descriptorType
			nullptr,  // pImageInfo
			array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
				vk::DescriptorBufferInfo(
					sameMatrixBuffer.get(),  // buffer
					0,  // offset
					numTriangles*16*sizeof(float)  // range
				),
			}.data(),
			nullptr  // pTexelBufferView
		),
		nullptr  // descriptorCopies
	);
	device->updateDescriptorSets(
		vk::WriteDescriptorSet(  // descriptorWrites
			transformationMatrixBufferDescriptorSet.get(),  // dstSet
			0,  // dstBinding
			0,  // dstArrayElement
			1,  // descriptorCount
			vk::DescriptorType::eStorageBuffer,  // descriptorType
			nullptr,  // pImageInfo
			array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
				vk::DescriptorBufferInfo(
					transformationMatrixBuffer.get(),  // buffer
					0,  // offset
					numTriangles*16*sizeof(float)  // range
				),
			}.data(),
			nullptr  // pTexelBufferView
		),
		nullptr  // descriptorCopies
	);


	// timestamp properties
	timestampValidBits=
		physicalDevice.getQueueFamilyProperties()[graphicsQueueFamily].timestampValidBits;
	timestampPeriod_ns=
		physicalDevice.getProperties().limits.timestampPeriod;
	cout<<"Timestamp number of bits: "<<timestampValidBits<<endl;
	cout<<"Timestamp period: "<<timestampPeriod_ns<<"ns"<<endl;
	if(timestampValidBits==0)
		throw runtime_error("Timestamps are not supported.");

	// timestamp pool
	timestampPool=
		device->createQueryPoolUnique(
			vk::QueryPoolCreateInfo(
				vk::QueryPoolCreateFlags(),  // flags
				vk::QueryType::eTimestamp,   // queryType
				uint32_t(tests.size())*2,    // queryCount
				vk::QueryPipelineStatisticFlags()  // pipelineStatistics
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

		// begin command buffer
		vk::CommandBuffer cb=commandBuffers[i].get();
		cb.begin(
			vk::CommandBufferBeginInfo(
				vk::CommandBufferUsageFlagBits::eSimultaneousUse,  // flags
				nullptr  // pInheritanceInfo
			)
		);
		cb.resetQueryPool(timestampPool.get(),0,uint32_t(tests.size())*2);
		uint32_t timestampIndex=0;

		// begin test lambda
		auto beginTest=
			[](vk::CommandBuffer cb,vk::Framebuffer framebuffer,vk::Extent2D currentSurfaceExtent,
			   vk::Pipeline pipeline,vk::PipelineLayout pipelineLayout,
			   const vector<vk::Buffer>& attributes,const vector<vk::DescriptorSet>& descriptorSets)
			{
				cb.beginRenderPass(
					vk::RenderPassBeginInfo(
						renderPass.get(),         // renderPass
						framebuffer,              // framebuffer
						vk::Rect2D(vk::Offset2D(0,0),currentSurfaceExtent),  // renderArea
						2,                        // clearValueCount
						array<vk::ClearValue,2>{  // pClearValues
							vk::ClearColorValue(array<float,4>{0.f,0.f,0.f,1.f}),
							vk::ClearDepthStencilValue(1.f,0)
						}.data()
					),
					vk::SubpassContents::eInline
				);
				cb.bindPipeline(vk::PipelineBindPoint::eGraphics,pipeline);  // bind pipeline
				cb.bindDescriptorSets(
					vk::PipelineBindPoint::eGraphics,  // pipelineBindPoint
					pipelineLayout,  // layout
					0,  // firstSet
					descriptorSets,  // descriptorSets
					nullptr  // dynamicOffsets
				);
				cb.bindVertexBuffers(
					0,  // firstBinding
					uint32_t(attributes.size()),  // bindingCount
					attributes.data(),  // pBuffers
					vector<vk::DeviceSize>(attributes.size(),0).data()  // pOffsets
				);
				cb.pipelineBarrier(
					vk::PipelineStageFlagBits::eAllCommands,  // srcStageMask
					vk::PipelineStageFlagBits::eTopOfPipe,  // dstStageMask
					vk::DependencyFlags(),  // dependencyFlags
					0,nullptr,  // memoryBarrierCount+pMemoryBarriers
					0,nullptr,  // bufferMemoryBarrierCount+pBufferMemoryBarriers
					0,nullptr   // imageMemoryBarrierCount+pImageMemoryBarriers
				);
			};

		// render something to put GPU out of power saving states
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          coordinateAttributePipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>{ coordinateAttribute.get() },
		          vector<vk::DescriptorSet>());
		cb.draw(3*numTriangles,1,0,0);
		cb.draw(3*numTriangles,1,0,0);
		cb.endRenderPass();

		// attributeless constant output test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          attributelessConstantOutputPipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// attributeless input indices test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          attributelessInputIndicesPipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// attributeless instancing test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          attributelessConstantOutputPipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3,numTriangles,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// coordinate attribute test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          coordinateAttributePipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>{ coordinateAttribute.get() },
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// coordinates in buffer test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          coordinateBufferPipeline.get(),coordinateBufferPipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>{ coordinateBufferDescriptorSet.get() });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// singleUniformMatrix test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          singleUniformMatrixPipeline.get(),singleUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ coordinateAttribute.get() },
		          vector<vk::DescriptorSet>{ singleUniformDescriptorSet.get() });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// matrixAttribute test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          matrixAttributePipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>{ coordinateAttribute.get(), transformationMatrixBuffer.get() },
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3,numTriangles,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// matrixBuffer test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          matrixBufferPipeline.get(),matrixBufferPipelineLayout.get(),
		          vector<vk::Buffer>{ coordinateAttribute.get() },
		          vector<vk::DescriptorSet>{ sameMatrixBufferDescriptorSet.get() });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// per-triangle draw call test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          coordinateAttributePipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>{ coordinateAttribute.get() },
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		for(size_t i=0; i<numTriangles; i++)
			cb.draw(3,1,uint32_t(i*3),0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// attributeless indirect buffer instancing
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          attributelessConstantOutputPipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.drawIndirect(indirectBuffer.get(),  // buffer
		                numTriangles*sizeof(vk::DrawIndirectCommand),  // offset
		                1,  // drawCount
		                sizeof(vk::DrawIndirectCommand));  // stride
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// attributeless indirect buffer per-triangle record
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          attributelessConstantOutputPipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.drawIndirect(indirectBuffer.get(),  // buffer
		                0,  // offset
		                numTriangles,  // drawCount
		                sizeof(vk::DrawIndirectCommand));  // stride
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// indirect buffer per-triangle record
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          coordinateAttributePipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>{ coordinateAttribute.get() },
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.drawIndirect(indirectBuffer.get(),  // buffer
		                0,  // offset
		                numTriangles,  // drawCount
		                sizeof(vk::DrawIndirectCommand));  // stride
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// end command buffer
		cb.end();
		assert(timestampIndex==tests.size()*2 && "Number of timestamps and number of tests mismatch.");
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
int main(int argc,char** argv)
{
	// catch exceptions
	// (vulkan.hpp fuctions throws if they fail)
	try {

		// init Vulkan and open window,
		// give physical device index as parameter
		init(argc>=2?size_t(max(atoi(argv[1]),0)):0);

		auto startTime=chrono::steady_clock::now();

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

					// avoid 0,0 surface extent as creation of swapchain would fail
					// (0,0 is returned on some platforms (particularly Windows) when window is minimized)
					if(surfaceCapabilities.currentExtent.width==0 || surfaceCapabilities.currentExtent.height==0)
						continue;

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
			graphicsQueue.waitIdle();

			// read timestamps
			vector<uint64_t> timestamps(tests.size()*2);
			device->getQueryPoolResults(
				timestampPool.get(),  // queryPool
				0,                    // firstQuery
				uint32_t(tests.size())*2,  // queryCount
				tests.size()*2*sizeof(uint64_t),  // dataSize
				timestamps.data(),    // pData
				sizeof(uint64_t),     // stride
				vk::QueryResultFlagBits::e64  // flags
			);
			size_t i=0;
			for(Test& t : tests) {
				t.renderingTimes.emplace_back(timestamps[i+1]-timestamps[i]);
				i+=2;
			}
			if(timestamps.begin()!=timestamps.end())
				for(auto it=timestamps.begin()+1; it!=timestamps.end(); it++)
					if(*(it-1)>*it)
						throw std::runtime_error("Tests ran in parallel.");

			// print the result at the end
			double totalMeasurementTime=chrono::duration<double>(chrono::steady_clock::now()-startTime).count();
			if(totalMeasurementTime>2.) {
				cout<<"Triangle throughput:"<<endl;
				for(Test& t : tests) {
					sort(t.renderingTimes.begin(),t.renderingTimes.end());
					double time_ns=t.renderingTimes[t.renderingTimes.size()/2]*timestampPeriod_ns;
					cout<<"   "<<t.resultString<<": "<<double(numTriangles)/time_ns*1e9/1e6<<" millions per second"<<endl;
				}
				cout<<"Time of a triangle:"<<endl;
				for(Test& t : tests) {
					double time_ns=t.renderingTimes[t.renderingTimes.size()/2]*timestampPeriod_ns;
					cout<<"   "<<t.resultString<<": "<<time_ns/numTriangles<<"ns"<<endl;
				}
				cout<<"Number of measurements of each test: "<<tests.front().renderingTimes.size()<<endl;
				cout<<"Total time of all measurements: "<<totalMeasurementTime<<" seconds"<<endl;
				break;
			}

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
