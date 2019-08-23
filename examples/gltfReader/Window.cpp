#include "Window.h"
#include <CadR/VulkanInstance.h>
#ifdef _WIN32
#else
# include <X11/Xutil.h>
#endif
#include <stdexcept>

using namespace std;
using namespace CadUI;


void CadUI::Window::init(CadR::VulkanInstance& instance)
{
#ifdef _WIN32

	// initial window size
	RECT screenSize;
	if(GetWindowRect(GetDesktopWindow(),&screenSize)==0)
		throw std::runtime_error("GetWindowRect() failed.");
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
		"Hello triangle",
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
	windowSize.setWidth(XWidthOfScreen(screen)/2);
	windowSize.setHeight(XHeightOfScreen(screen)/2);
	window=XCreateSimpleWindow(display,DefaultRootWindow(display),0,0,windowSize.width,
	                           windowSize.height,0,blackColor,blackColor);
	XSetStandardProperties(display,window,"Hello triangle",NULL,None,NULL,0,NULL);
	XSelectInput(display,window,StructureNotifyMask);
	wmDeleteMessage=XInternAtom(display,"WM_DELETE_WINDOW",False);
	XSetWMProtocols(display,window,&wmDeleteMessage,1);
	XMapWindow(display,window);

	// create surface
	_instance=&instance;
	vkCreateXlibSurfaceKHR=_instance->getProcAddr<PFN_vkCreateXlibSurfaceKHR>("vkCreateXlibSurfaceKHR");
	vkDestroySurfaceKHR=_instance->getProcAddr<PFN_vkDestroySurfaceKHR>("vkDestroySurfaceKHR");
	_surface=(*_instance)->createXlibSurfaceKHR(vk::XlibSurfaceCreateInfoKHR(vk::XlibSurfaceCreateFlagsKHR(),display,window),nullptr,*this);

#endif
}


CadUI::Window::~Window()
{
	if(_surface) {
		(*_instance)->destroy(_surface,nullptr,*this);
		_surface=nullptr;
	}

#ifdef _WIN32
	if(window) {
		DestroyWindow(window);
		UnregisterClass("RenderingWindow",GetModuleHandle(NULL));
	}
#else
	if(window)  XDestroyWindow(display,window);
	if(display)  XCloseDisplay(display);
#endif
}
