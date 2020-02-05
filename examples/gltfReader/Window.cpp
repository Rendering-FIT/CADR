#ifdef _WIN32
# define VK_USE_PLATFORM_WIN32_KHR
#endif
#include "Window.h"
#include <CadR/VulkanDevice.h>
#include <CadR/VulkanInstance.h>
#ifndef _WIN32
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
		auto handleMouseButton=[](HWND hwnd,ButtonEventType t,MouseButtons changedButton,LPARAM lParam,WPARAM wParam) {
			Window* w=(Window*)GetWindowLongPtr(hwnd,0);
			w->mouseButtonCallback.invoke(
				t,                      // eventType
				changedButton,          // changedButton
				short(LOWORD(lParam)),  // x
				short(HIWORD(lParam)),  // y
				MouseButtons(           // newButtonState
					(wParam&MK_LBUTTON?MouseButtons::Left:0) |
					(wParam&MK_MBUTTON?MouseButtons::Middle:0) |
					(wParam&MK_RBUTTON?MouseButtons::Right:0)
				)
			);
		};
		switch(msg)
		{
			case WM_MOUSEMOVE: {
				Window* w=(Window*)GetWindowLongPtr(hwnd,0);
				w->mouseMoveCallback.invoke(
					short(LOWORD(lParam)),  // x
					short(HIWORD(lParam)),  // y
					MouseButtons(           // buttonState
						(wParam&MK_LBUTTON?MouseButtons::Left:0) |
						(wParam&MK_MBUTTON?MouseButtons::Middle:0) |
						(wParam&MK_RBUTTON?MouseButtons::Right:0)
					)
				);
				return 0;
			}
			case WM_LBUTTONDOWN:
				handleMouseButton(hwnd,ButtonEventType::Pressed,MouseButtons::Left,lParam,wParam);
				return 0;
			case WM_LBUTTONUP:
				handleMouseButton(hwnd,ButtonEventType::Released,MouseButtons::Left,lParam,wParam);
				return 0;
			case WM_MBUTTONDOWN:
				handleMouseButton(hwnd,ButtonEventType::Pressed,MouseButtons::Middle,lParam,wParam);
				return 0;
			case WM_MBUTTONUP:
				handleMouseButton(hwnd,ButtonEventType::Released,MouseButtons::Middle,lParam,wParam);
				return 0;
			case WM_RBUTTONDOWN:
				handleMouseButton(hwnd,ButtonEventType::Pressed,MouseButtons::Right,lParam,wParam);
				return 0;
			case WM_RBUTTONUP:
				handleMouseButton(hwnd,ButtonEventType::Released,MouseButtons::Right,lParam,wParam);
				return 0;
			case WM_MOUSEWHEEL: {
				Window* w=(Window*)GetWindowLongPtr(hwnd,0);
				w->mouseWheelCallback.invoke(
					GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA,  // z-delta
					short(LOWORD(lParam)),  // x
					short(HIWORD(lParam)),  // y
					MouseButtons(           // buttonState
						(wParam&MK_LBUTTON?MouseButtons::Left:0) |
						(wParam&MK_MBUTTON?MouseButtons::Middle:0) |
						(wParam&MK_RBUTTON?MouseButtons::Right:0)
					)
				);
				return 0;
			}
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
	ShowWindow(reinterpret_cast<HWND>(window),SW_SHOWDEFAULT);

	// create surface
	_instance=&instance;
	vkCreateWin32SurfaceKHR=_instance->getProcAddr<PFN_vkCreateWin32SurfaceKHR>("vkCreateWin32SurfaceKHR");
	_surface=
		(*_instance)->createWin32SurfaceKHR(
			vk::Win32SurfaceCreateInfoKHR(
				vk::Win32SurfaceCreateFlagsKHR(),  // flags
				wc.hInstance,                      // hinstance
				reinterpret_cast<HWND>(window)     // hwnd
			),
			nullptr,  // allocator
			*this  // dispatch
		);

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
	XSelectInput(display,window,StructureNotifyMask|PointerMotionMask|ButtonPressMask|ButtonReleaseMask);
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
		DestroyWindow(reinterpret_cast<HWND>(window));
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
                               uint32_t presentationQueueFamily,vk::RenderPass renderPass,
                               vk::PresentModeKHR presentMode)
{
	if(_device)
		cleanUpVulkan();

	// initialize device-level function pointers
	_physicalDevice=physicalDevice;
	_device=&device;
	_surfaceFormat=surfaceFormat;
	_graphicsQueueFamily=graphicsQueueFamily;
	_presentationQueueFamily=presentationQueueFamily;
	_renderPass=renderPass;
	_presentMode=presentMode;
	vkCreateSwapchainKHR=_device->getProcAddr<PFN_vkCreateSwapchainKHR>("vkCreateSwapchainKHR");
	vkDestroySwapchainKHR=_device->getProcAddr<PFN_vkDestroySwapchainKHR>("vkDestroySwapchainKHR");
	vkGetSwapchainImagesKHR=_device->getProcAddr<PFN_vkGetSwapchainImagesKHR>("vkGetSwapchainImagesKHR");
	vkAcquireNextImageKHR=_device->getProcAddr<PFN_vkAcquireNextImageKHR>("vkAcquireNextImageKHR");
	vkQueuePresentKHR=_device->getProcAddr<PFN_vkQueuePresentKHR>("vkQueuePresentKHR");

	// register cleanUp handler
	_device->cleanUpCallbacks.append(&Window::cleanUpVulkan,this);
}


void CadUI::Window::cleanUpVulkan()
{
	// do not do anything if Window::initVulkan was not called
	if(!_device)
		return;

	// unregister cleanUp handler
	_device->cleanUpCallbacks.remove(&Window::cleanUpVulkan,this);

	// clean up all device-level Vulkan stuff
	if(_swapchainImageViews.size()>0) {
		for(auto v:_swapchainImageViews)
			_device->destroy(v);
		_swapchainImageViews.clear();
	}
	if(_framebuffers.size()>0) {
		for(auto f:_framebuffers)
			_device->destroy(f);
		_framebuffers.clear();
	}
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

		// handle resize event
		if(e.type==ConfigureNotify && e.xconfigure.window==window) {
			vk::Extent2D newSize(e.xconfigure.width,e.xconfigure.height);
			if(newSize!=_windowSize) {
				_needResize=true;
				_windowSize=newSize;
			}
			continue;
		}

		// handle mouse move
		if(e.type==MotionNotify && e.xmotion.window==window) {
			mouseMoveCallback.invoke(
				e.xmotion.x,
				e.xmotion.y,
				MouseButtons(
					(e.xmotion.state&Button1Mask?MouseButtons::Left:0) |
					(e.xmotion.state&Button2Mask?MouseButtons::Middle:0) |
					(e.xmotion.state&Button3Mask?MouseButtons::Right:0)
				)
			);
			continue;
		}

		// handle mouse buttons
		if((e.type==ButtonPress || e.type==ButtonRelease) && e.xbutton.window==window) {

			// state of mouse buttons
			MouseButtons buttonState(
				(e.xbutton.state&Button1Mask?MouseButtons::Left:0) |
				(e.xbutton.state&Button2Mask?MouseButtons::Middle:0) |
				(e.xbutton.state&Button3Mask?MouseButtons::Right:0)
			);

			// handle mouse wheel
			if(e.xbutton.button==Button4) {
				if(e.xbutton.type==ButtonPress)
					mouseWheelCallback.invoke(+1,e.xbutton.x,e.xbutton.y,buttonState);
				continue;
			}
			if(e.xbutton.button==Button5) {
				if(e.xbutton.type==ButtonPress)
					mouseWheelCallback.invoke(-1,e.xbutton.x,e.xbutton.y,buttonState);
				continue;
			}

			// changed button
			MouseButtons changedButton(
				e.xbutton.button==Button1 ? MouseButtons::Left :
					e.xbutton.button==Button3 ? MouseButtons::Right :
						e.xbutton.button==Button2 ? MouseButtons::Middle : MouseButtons::Unknown
			);
			if(e.type==ButtonPress)  buttonState|=changedButton;
			else  buttonState&=~changedButton;

			// invoke callback
			mouseButtonCallback.invoke(
				e.type==ButtonPress ? ButtonEventType::Pressed : ButtonEventType::Released,
				changedButton,
				e.xbutton.x,
				e.xbutton.y,
				buttonState
			);

			continue;
		}

		// handle window close
		if(e.type==ClientMessage && ulong(e.xclient.data.l[0])==wmDeleteMessage)
			return false;
	}

#endif

	return true;
}


bool CadUI::Window::updateSize()
{
	// recreate swapchain if necessary
	if(!_needResize)
		return true;

	// check if proper initialization took place
	if(!_device)
		throw std::runtime_error("CadUI::Window::updateSize() called without proper call to Window::initVulkan().");

	// recreate only upon surface extent change
	vk::SurfaceCapabilitiesKHR surfaceCapabilities(_physicalDevice.getSurfaceCapabilitiesKHR(_surface,*this));
	if(surfaceCapabilities.currentExtent!=_surfaceExtent) {

		// avoid 0,0 surface extent as creation of swapchain would fail
		// (0,0 is returned on some platforms (particularly Windows) when window is minimized)
		if(surfaceCapabilities.currentExtent.width==0 || surfaceCapabilities.currentExtent.height==0)
			return false;

		// recreate swapchain
		recreateSwapchain(surfaceCapabilities);
	}

	_needResize=false;
	return true;
}


void CadUI::Window::recreateSwapchain()
{
	recreateSwapchain(_physicalDevice.getSurfaceCapabilitiesKHR(_surface,*this));
}


void CadUI::Window::recreateSwapchain(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities)
{
	// set new surface extent
	_surfaceExtent=
		(surfaceCapabilities.currentExtent.width==std::numeric_limits<uint32_t>::max())
			?vk::Extent2D{std::clamp(_windowSize.width ,surfaceCapabilities.minImageExtent.width ,surfaceCapabilities.maxImageExtent.width),
			              std::clamp(_windowSize.height,surfaceCapabilities.minImageExtent.height,surfaceCapabilities.maxImageExtent.height)}
			:surfaceCapabilities.currentExtent;

	// save old swapchain
	auto oldSwapchain=
		vk::UniqueHandle<vk::SwapchainKHR,CadUI::Window>{
			_swapchain,  // value
			vk::ObjectDestroy<vk::Device,CadUI::Window>(*_device,nullptr,*this)  // deleter
		};
	_swapchain=nullptr;

	// create new swapchain
	_swapchain=
		(*_device)->createSwapchainKHR(
			vk::SwapchainCreateInfoKHR(
				vk::SwapchainCreateFlagsKHR(),  // flags
				_surface,                       // surface
				surfaceCapabilities.maxImageCount==0  // minImageCount
					?surfaceCapabilities.minImageCount+1
					:min(surfaceCapabilities.minImageCount+1,surfaceCapabilities.maxImageCount),
				_surfaceFormat.format,          // imageFormat
				_surfaceFormat.colorSpace,      // imageColorSpace
				_surfaceExtent,                 // imageExtent
				1,                              // imageArrayLayers
				vk::ImageUsageFlagBits::eColorAttachment,  // imageUsage
				(_graphicsQueueFamily==_presentationQueueFamily)?vk::SharingMode::eExclusive:vk::SharingMode::eConcurrent, // imageSharingMode
				(_graphicsQueueFamily==_presentationQueueFamily)?uint32_t(0):uint32_t(2),  // queueFamilyIndexCount
				(_graphicsQueueFamily==_presentationQueueFamily)?nullptr:array<uint32_t,2>{_graphicsQueueFamily,_presentationQueueFamily}.data(),  // pQueueFamilyIndices
				surfaceCapabilities.currentTransform,    // preTransform
				vk::CompositeAlphaFlagBitsKHR::eOpaque,  // compositeAlpha
				_presentMode,  // presentMode
				VK_TRUE,       // clipped
				oldSwapchain.get()  // oldSwapchain
			),
			nullptr,  // allocator
			*this     // dispatch
		);

	// destroy old swapchain
	oldSwapchain.reset();

	// clear various Vulkan objects
	if(_framebuffers.size()>0) {
		for(auto f:_framebuffers)
			_device->destroy(f);
		_framebuffers.clear();
	}
	if(_swapchainImageViews.size()>0) {
		for(auto v:_swapchainImageViews)
			_device->destroy(v);
		_swapchainImageViews.clear();
	}

	// swapchain images and image views
	vector<vk::Image> swapchainImages=(*_device)->getSwapchainImagesKHR(_swapchain,*this);
	_swapchainImageViews.reserve(swapchainImages.size());
	for(vk::Image image:swapchainImages)
		_swapchainImageViews.emplace_back(
			_device->createImageView(
				vk::ImageViewCreateInfo(
					vk::ImageViewCreateFlags(),  // flags
					image,                       // image
					vk::ImageViewType::e2D,      // viewType
					_surfaceFormat.format,       // format
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

	// framebuffers
	_framebuffers.reserve(swapchainImages.size());
	for(size_t i=0,c=swapchainImages.size(); i<c; i++)
		_framebuffers.emplace_back(
			_device->createFramebuffer(
				vk::FramebufferCreateInfo(
					vk::FramebufferCreateFlags(),   // flags
					_renderPass,                    // renderPass
					1,  // attachmentCount
					&_swapchainImageViews[i],       // pAttachments
					_surfaceExtent.width,           // width
					_surfaceExtent.height,          // height
					1  // layers
				)
			)
		);

	// call resize callbacks
	resizeCallbacks.invoke();
}


tuple<uint32_t,bool> CadUI::Window::acquireNextImage(vk::Semaphore semaphoreToSignal,vk::Fence fenceToSignal)
{
	uint32_t imageIndex;
	vk::Result r=
		(*_device)->acquireNextImageKHR(
			_swapchain,                       // swapchain
			numeric_limits<uint64_t>::max(),  // timeout
			semaphoreToSignal,                // semaphore to signal
			fenceToSignal,                    // fence to signal
			&imageIndex,                      // pImageIndex
			*this                             // dispatch
		);
	if(r!=vk::Result::eSuccess) {
		if(r==vk::Result::eErrorOutOfDateKHR||r==vk::Result::eSuboptimalKHR) { _needResize=true; return {0,false}; }
		vk::throwResultException(r,VULKAN_HPP_NAMESPACE_STRING"::Device::acquireNextImageKHR");
	}
	return {imageIndex,true};
}


bool CadUI::Window::present(vk::Queue presentationQueue,vk::Semaphore semaphoreToWait,uint32_t imageIndex)
{
	vk::Result r=
		presentationQueue.presentKHR(
			&(const vk::PresentInfoKHR&)vk::PresentInfoKHR(
				1,&semaphoreToWait,         // waitSemaphoreCount+pWaitSemaphores
				1,&_swapchain,&imageIndex,  // swapchainCount+pSwapchains+pImageIndices
				nullptr                     // pResults
			),
			*this  // dispatch
		);
	if(r!=vk::Result::eSuccess) {
		if(r==vk::Result::eErrorOutOfDateKHR||r==vk::Result::eSuboptimalKHR) { _needResize=true; return false; }
		else  vk::throwResultException(r,VULKAN_HPP_NAMESPACE_STRING"::Queue::presentKHR");
	}
	return true;
}
