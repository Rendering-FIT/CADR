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
static HWND w=nullptr;
#endif


int main(int,char**)
{
#ifndef _WIN32
	Display* d=nullptr;
	Window w=0;
#endif

	// catch exceptions
	// (vulkan.hpp fuctions throws if they fail)
	try {

#ifndef _WIN32
		// open X connection
		d=XOpenDisplay(nullptr);
		if(d==nullptr)
			throw runtime_error("Can not open display. No X-server running or wrong DISPLAY variable.");
#endif

		// Vulkan instance
		vk::UniqueInstance instance=
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
					//array<const char*,2>{{"VK_KHR_surface","VK_KHR_xlib_surface"}}.data(),  // enabled extension names
					array<const char*,2>{{"VK_KHR_surface","VK_KHR_win32_surface"}}.data(),  // enabled extension names
				});

#ifdef _WIN32

		// window's message handling procedure
		auto wndProc=[](HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)->LRESULT {
			switch(msg)
			{
				case WM_CLOSE:
					DestroyWindow(hwnd);
					w=nullptr;
				break;
				case WM_DESTROY:
					PostQuitMessage(0);
				break;
				default:
					return DefWindowProc(hwnd,msg,wParam,lParam);
			}
			return 0;
		};

		// register window class
		WNDCLASSEX wc;
		wc.cbSize        = sizeof(WNDCLASSEX);
		wc.style         = 0;
		wc.lpfnWndProc   = wndProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = GetModuleHandle(NULL);
		wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = "HelloWindow";
		wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
		if(!RegisterClassEx(&wc))
			throw runtime_error("Can not register window class.");

		// create window
		w=CreateWindowEx(
			WS_EX_CLIENTEDGE,
			"HelloWindow",
			"Hello window!",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,CW_USEDEFAULT,240,120,
			NULL,NULL,wc.hInstance,NULL);
		if(w==NULL)
			throw runtime_error("Can not create window.");
		ShowWindow(w,SW_SHOWDEFAULT);
		UpdateWindow(w);

		// create surface
		vk::UniqueSurfaceKHR s=instance->createWin32SurfaceKHRUnique(vk::Win32SurfaceCreateInfoKHR(vk::Win32SurfaceCreateFlagsKHR(),wc.hInstance,w));

#else

		// create window
		int blackColor=BlackPixel(d,DefaultScreen(d));
		Screen* screen=XDefaultScreenOfDisplay(d);
		w=XCreateSimpleWindow(d,DefaultRootWindow(d),0,0,XWidthOfScreen(screen)/2,
		                      XHeightOfScreen(screen)/2,0,blackColor,blackColor);
		XSetStandardProperties(d,w,"Hello window!","Hello window!",None,NULL,0,NULL);
		Atom wmDeleteMessage=XInternAtom(d,"WM_DELETE_WINDOW",False);
		XSetWMProtocols(d,w,&wmDeleteMessage,1);
		XMapWindow(d,w);

		// create surface
		vk::UniqueHandle<vk::SurfaceKHR> s=instance->createXlibSurfaceKHRUnique(vk::XlibSurfaceCreateInfoKHR(vk::XlibSurfaceCreateFlagsKHR(),d,w));

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
				if(strcmp(e.extensionName,VK_KHR_SWAPCHAIN_EXTENSION_NAME)==0)
					goto swapchainSupported;
			continue;
			swapchainSupported:

			// skip devices without surface formats and presentation modes
			uint32_t formatCount;
			vk::createResultValue(pd.getSurfaceFormatsKHR(s.get(),&formatCount,nullptr),VULKAN_HPP_NAMESPACE_STRING"::PhysicalDevice::getSurfaceFormatsKHR");
			uint32_t presentationModeCount;
			vk::createResultValue(pd.getSurfacePresentModesKHR(s.get(),&presentationModeCount,nullptr),VULKAN_HPP_NAMESPACE_STRING"::PhysicalDevice::getSurfacePresentModesKHR");
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
				bool p=pd.getSurfaceSupportKHR(i,s.get())!=0;
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

		/*for(vk::PhysicalDevice& pd:compatibleDevices) {
			auto formatList=pd.getSurfaceFormatsKHR(s.get());
			//for(auto f:formatList)
				//cout<<vk::to_string(f)<<endl;
			auto presentModeList=pd.getSurfacePresentModesKHR(s.get());
			for(auto p:presentModeList)
				cout<<vk::to_string(p)<<endl;
		}*/

		// choose device
		vk::PhysicalDevice pd;
		uint32_t graphicsQueueFamily,presentationQueueFamily;
		if(compatibleDevicesSingleQueue.size()>0) {
			auto t=compatibleDevicesSingleQueue.front();
			pd=get<0>(t);
			graphicsQueueFamily=get<1>(t);
			presentationQueueFamily=UINT32_MAX;
		}
		else if(compatibleDevicesTwoQueues.size()>0) {
			auto t=compatibleDevicesTwoQueues.front();
			pd=get<0>(t);
			graphicsQueueFamily=get<1>(t);
			presentationQueueFamily=get<2>(t);
		}
		else {
			cout<<"No compatible devices. Exiting..."<<endl;
			return 1;
		}
		cout<<"Using device:\n"
		      "   "<<pd.getProperties().deviceName<<endl;

		// create device
		vk::UniqueDevice device=pd.createDeviceUnique(
			vk::DeviceCreateInfo{
				vk::DeviceCreateFlags(),  // flags
				compatibleDevicesSingleQueue.size()>0?uint32_t(1):uint32_t(2),  // queueCreateInfoCount
				array<const vk::DeviceQueueCreateInfo,2>{  // pQueueCreateInfos
					vk::DeviceQueueCreateInfo{
						vk::DeviceQueueCreateFlags(),
						graphicsQueueFamily,
						1,
						&(const float&)1.f,
					},vk::DeviceQueueCreateInfo{
						vk::DeviceQueueCreateFlags(),
						presentationQueueFamily,
						1,
						&(const float&)1.f,
					}
				}.data(),
				0,nullptr,  // no layers
				1,          // number of enabled extensions
				array<const char*,1>{{VK_KHR_SWAPCHAIN_EXTENSION_NAME}}.data(),  // enabled extension names
				nullptr,    // enabled features
			}
		);

		//

		// create swapchain


		// run event loop
#ifdef _WIN32
		MSG msg;
		while(GetMessage(&msg,NULL,0,0)>0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
#else
		while(true) {
			XEvent e;
			XNextEvent(d,&e);
			if(e.type==ClientMessage&&ulong(e.xclient.data.l[0])==wmDeleteMessage)
				break;
		}
#endif

	// catch exceptions
	} catch(vk::Error &e) {
		cout<<"Failed because of Vulkan exception: "<<e.what()<<endl;
	} catch(exception &e) {
		cout<<"Failed because of exception: "<<e.what()<<endl;
	} catch(...) {
		cout<<"Failed because of unspecified exception."<<endl;
	}

	// clean up
#ifdef _WIN32
	if(w)
		DestroyWindow(w);
#else
	if(w)
		XDestroyWindow(d,w);
	if(d)
		XCloseDisplay(d);
#endif

	return 0;
}
