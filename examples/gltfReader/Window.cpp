#include "Window.h"
#include <CadR/VulkanDevice.h>
#include <CadR/VulkanInstance.h>
#ifdef _WIN32
#else
# include <X11/Xutil.h>
#endif
#include <stdexcept>

using namespace std;
using namespace CadUI;


CadUI::Window::Window(Window&& other)
	: Window(other)
{
#ifdef _WIN32
	other.window=nullptr;
#else
	other.display=nullptr;
	other.window=0;
#endif
	other._instance=nullptr;
	other._surface=nullptr;
	other._swapchain=nullptr;
	other._physicalDevice=nullptr;
	other._device=nullptr;
}


CadUI::Window::~Window()
{
	destroy();
}


CadUI::Window& CadUI::Window::operator=(CadUI::Window&& rhs)
{
	*this=rhs;

#ifdef _WIN32
	rhs.window=nullptr;
#else
	rhs.display=nullptr;
	rhs.window=0;
#endif
	rhs._instance=nullptr;
	rhs._surface=nullptr;
	rhs._swapchain=nullptr;
	rhs._physicalDevice=nullptr;
	rhs._device=nullptr;

	return *this;
}


void CadUI::Window::create(CadR::VulkanInstance& instance)
{
	if(_instance==&instance)
		return;
	if(_instance)
		destroy();

#ifdef _WIN32

	// initial window size
	RECT screenSize;
	if(GetWindowRect(GetDesktopWindow(),&screenSize)==0)
		throw std::runtime_error("GetWindowRect() failed.");
	_windowSize.setWidth((screenSize.right-screenSize.left)/2);
	_windowSize.setHeight((screenSize.bottom-screenSize.top)/2);

	// window's message handling procedure
	auto wndProc=[](HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)->LRESULT {
		switch(msg)
		{
			case WM_SIZE: {
				Window* w=(Window*)GetWindowLongPtr(hwnd,0);
				w->_needResize=true;
				w->_windowSize.setWidth(LOWORD(lParam));
				w->_windowSize.setHeight(HIWORD(lParam));
				return DefWindowProc(hwnd,msg,wParam,lParam);
			}
			case WM_CLOSE:
				DestroyWindow(hwnd);
				UnregisterClass("RenderingWindow",GetModuleHandle(NULL));
				return 0;
			case WM_NCCREATE:
				SetWindowLongPtr(hwnd,0,(LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
				return DefWindowProc(hwnd,msg,wParam,lParam);
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
	wc.cbWndExtra    = sizeof(LONG_PTR);
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
		"Hello triangle",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,CW_USEDEFAULT,_windowSize.width,_windowSize.height,
		NULL,NULL,wc.hInstance,this);
	if(window==NULL) {
		UnregisterClass("RenderingWindow",GetModuleHandle(NULL));
		throw runtime_error("Can not create window.");
	}

	// show window
	ShowWindow(window,SW_SHOWDEFAULT);

	// create surface
	_instance=&instance;
	vkCreateWin32SurfaceKHR=_instance->getProcAddr<PFN_vkCreateWin32SurfaceKHR>("vkCreateWin32SurfaceKHR");
	_surface=(*_instance)->createWin32SurfaceKHR(vk::Win32SurfaceCreateInfoKHR(vk::Win32SurfaceCreateFlagsKHR(),wc.hInstance,window),nullptr,*this);

#else

	// open X connection
	display=XOpenDisplay(nullptr);
	if(display==nullptr)
		throw runtime_error("Can not open display. No X-server running or wrong DISPLAY variable.");

	// create window
	int blackColor=BlackPixel(display,DefaultScreen(display));
	Screen* screen=XDefaultScreenOfDisplay(display);
	_windowSize.setWidth(XWidthOfScreen(screen)/2);
	_windowSize.setHeight(XHeightOfScreen(screen)/2);
	window=XCreateSimpleWindow(display,DefaultRootWindow(display),0,0,_windowSize.width,
	                           _windowSize.height,0,blackColor,blackColor);
	XSetStandardProperties(display,window,"Hello triangle",NULL,None,NULL,0,NULL);
	XSelectInput(display,window,StructureNotifyMask);
	wmDeleteMessage=XInternAtom(display,"WM_DELETE_WINDOW",False);
	XSetWMProtocols(display,window,&wmDeleteMessage,1);
	XMapWindow(display,window);

	// create surface
	_instance=&instance;
	vkCreateXlibSurfaceKHR=_instance->getProcAddr<PFN_vkCreateXlibSurfaceKHR>("vkCreateXlibSurfaceKHR");
	_surface=(*_instance)->createXlibSurfaceKHR(vk::XlibSurfaceCreateInfoKHR(vk::XlibSurfaceCreateFlagsKHR(),display,window),nullptr,*this);

#endif

	// initialize instance-level function pointers
	vkDestroySurfaceKHR=_instance->getProcAddr<PFN_vkDestroySurfaceKHR>("vkDestroySurfaceKHR");
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR=_instance->getProcAddr<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>("vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
	vkGetPhysicalDeviceSurfacePresentModesKHR=_instance->getProcAddr<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>("vkGetPhysicalDeviceSurfacePresentModesKHR");
}


void CadUI::Window::destroy()
{
	cleanUpVulkan();

	if(_surface) {
		(*_instance)->destroy(_surface,nullptr,*this);
		_surface=nullptr;
	}
	_instance=nullptr;

#ifdef _WIN32
	if(window) {
		DestroyWindow(window);
		UnregisterClass("RenderingWindow",GetModuleHandle(NULL));
		window=nullptr;
	}
#else
	if(window)  { XDestroyWindow(display,window); window=0; }
	if(display)  { XCloseDisplay(display); display=nullptr; }
#endif
}


void CadUI::Window::initVulkan(vk::PhysicalDevice physicalDevice,CadR::VulkanDevice& device,
                               vk::SurfaceFormatKHR surfaceFormat,uint32_t graphicsQueueFamily,
                               uint32_t presentationQueueFamily,vk::PresentModeKHR presentMode)
{
	if(_device)
		cleanUpVulkan();

	// initialize device-level function pointers
	_physicalDevice=physicalDevice;
	_device=&device;
	_surfaceFormat=surfaceFormat;
	_graphicsQueueFamily=graphicsQueueFamily;
	_presentationQueueFamily=presentationQueueFamily;
	_presentMode=presentMode;
	vkCreateSwapchainKHR=_device->getProcAddr<PFN_vkCreateSwapchainKHR>("vkCreateSwapchainKHR");
	vkDestroySwapchainKHR=_device->getProcAddr<PFN_vkDestroySwapchainKHR>("vkDestroySwapchainKHR");

	// register cleanUp handler
	_device->addCleanUpHandler(&Window::cleanUpVulkan,this);
}


void CadUI::Window::cleanUpVulkan()
{
	// do not do anything if Window::initVulkan was not called
	if(!_device)
		return;

	// unregister cleanUp handler
	_device->removeCleanUpHandler(&Window::cleanUpVulkan,this);

	// clean up all device-level Vulkan stuff
	if(_swapchain) {
		(*_device)->destroy(_swapchain,nullptr,*this);
		_swapchain=nullptr;
	}

	_physicalDevice=nullptr;
	_device=nullptr;
}


bool CadUI::Window::processEvents()
{
	if(!_device)
		throw std::runtime_error("CadUI::Window::processEvents() called without proper call to Window::initVulkan().");

#ifdef _WIN32

	// process messages
	MSG msg;
	while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)>0) {
		if(msg.message==WM_QUIT)
			return false;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

#else

	// process messages
	XEvent e;
	while(XPending(display)>0) {
		XNextEvent(display,&e);
		if(e.type==ConfigureNotify && e.xconfigure.window==window) {
			vk::Extent2D newSize(e.xconfigure.width,e.xconfigure.height);
			if(newSize!=_windowSize) {
				_needResize=true;
				_windowSize=newSize;
			}
			continue;
		}
		if(e.type==ClientMessage && ulong(e.xclient.data.l[0])==wmDeleteMessage)
			return false;
	}

#endif

	return true;
}


bool CadUI::Window::updateSize()
{
	// recreate swapchain if necessary
	if(_needResize==false)
		return true;

	// check if proper initialization took place
	if(!_device)
		throw std::runtime_error("CadUI::Window::updateSize() called without proper call to Window::initVulkan().");

	// recreate only upon surface extent change
	vk::SurfaceCapabilitiesKHR surfaceCapabilities(_physicalDevice.getSurfaceCapabilitiesKHR(_surface,*this));
	if(surfaceCapabilities.currentExtent!=_currentSurfaceExtent) {

		// avoid 0,0 surface extent as creation of swapchain would fail
		// (0,0 is returned on some platforms (particularly Windows) when window is minimized)
		if(surfaceCapabilities.currentExtent.width==0 || surfaceCapabilities.currentExtent.height==0)
			return false;

		// new _currentSurfaceExtent
		_currentSurfaceExtent=
			(surfaceCapabilities.currentExtent.width!=std::numeric_limits<uint32_t>::max())
				?surfaceCapabilities.currentExtent
				:vk::Extent2D{max(min(_windowSize.width,surfaceCapabilities.maxImageExtent.width),surfaceCapabilities.minImageExtent.width),
				              max(min(_windowSize.height,surfaceCapabilities.maxImageExtent.height),surfaceCapabilities.minImageExtent.height)};

		// recreate swapchain
		recreateSwapchain();
	}

	_needResize=false;
	return true;
}


void CadUI::Window::recreateSwapchain()
{
	// create new swapchain
	vk::SurfaceCapabilitiesKHR surfaceCapabilities=_physicalDevice.getSurfaceCapabilitiesKHR(_surface,*this);
	vk::SwapchainKHR oldSwapchain=_swapchain;
	_swapchain=
		(*_device)->createSwapchainKHR(
			vk::SwapchainCreateInfoKHR(
				vk::SwapchainCreateFlagsKHR(),   // flags
				_surface,                        // surface
				surfaceCapabilities.maxImageCount==0  // minImageCount
					?surfaceCapabilities.minImageCount+1
					:min(surfaceCapabilities.minImageCount+1,surfaceCapabilities.maxImageCount),
				_surfaceFormat.format,           // imageFormat
				_surfaceFormat.colorSpace,       // imageColorSpace
				_currentSurfaceExtent,           // imageExtent
				1,  // imageArrayLayers
				vk::ImageUsageFlagBits::eColorAttachment,  // imageUsage
				(_graphicsQueueFamily==_presentationQueueFamily)?vk::SharingMode::eExclusive:vk::SharingMode::eConcurrent, // imageSharingMode
				(_graphicsQueueFamily==_presentationQueueFamily)?uint32_t(0):uint32_t(2),  // queueFamilyIndexCount
				(_graphicsQueueFamily==_presentationQueueFamily)?nullptr:array<uint32_t,2>{_graphicsQueueFamily,_presentationQueueFamily}.data(),  // pQueueFamilyIndices
				surfaceCapabilities.currentTransform,    // preTransform
				vk::CompositeAlphaFlagBitsKHR::eOpaque,  // compositeAlpha
				_presentMode,  // presentMode
				VK_TRUE,       // clipped
				oldSwapchain   // oldSwapchain
			),
			nullptr,
			*this
		);

	// destroy old swapchain
	if(oldSwapchain)
		(*_device)->destroy(oldSwapchain,nullptr,*this);
}
