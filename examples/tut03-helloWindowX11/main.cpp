#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.hpp>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <array>
#include <iostream>

using namespace std;


int main(int,char**)
{
	Display* d=nullptr;

	// catch exceptions
	// (vulkan.hpp fuctions throws if they fail)
	try {

		// open X connection
		d=XOpenDisplay(nullptr);
		if(d==nullptr)
			throw runtime_error("Can not open display. No X-server running or wrong DISPLAY variable.");

		// Vulkan instance
		vk::UniqueHandle<vk::Instance> instance(
			vk::createInstance(
				vk::InstanceCreateInfo{
					vk::InstanceCreateFlags(),  // flags
					&(const vk::ApplicationInfo&)vk::ApplicationInfo{
						"CADR tut01",            // application name
						VK_MAKE_VERSION(0,0,0),  // application version
						"CADR",                  // engine name
						VK_MAKE_VERSION(0,0,0),  // engine version
						VK_API_VERSION_1_0,      // api version
					},
#ifdef NDEBUG
					0, nullptr, // no debug layers
#else
					3,          // enabled layer count
					array<const char*,3>{{"VK_LAYER_LUNARG_standard_validation",
						                   "VK_LAYER_LUNARG_core_validation",
					                      "VK_LAYER_LUNARG_parameter_validation"}}.data(),  // enabled layer names
#endif
					2,        // enabled extension count
					array<const char*,2>{{"VK_KHR_surface","VK_KHR_xlib_surface"}}.data(),  // enabled extension names
				}));

		// create window
		int blackColor=BlackPixel(d,DefaultScreen(d));
		Screen* screen=XDefaultScreenOfDisplay(d);
		Window w=XCreateSimpleWindow(d,DefaultRootWindow(d),0,0,XWidthOfScreen(screen)/2,
		                             XHeightOfScreen(screen)/2,0,blackColor,blackColor);
		XSetStandardProperties(d,w,"Hello window!","Hello window!",None,NULL,0,NULL);
		Atom wmDeleteMessage=XInternAtom(d,"WM_DELETE_WINDOW",False);
		XSetWMProtocols(d,w,&wmDeleteMessage,1);
		XMapWindow(d,w);

		// create surface
		vk::UniqueHandle<vk::SurfaceKHR> s=instance->createXlibSurfaceKHRUnique(vk::XlibSurfaceCreateInfoKHR(vk::XlibSurfaceCreateFlagsKHR(),d,w));

		// get VisualID
		XWindowAttributes a;
		XGetWindowAttributes(d,w,&a);
		VisualID v=XVisualIDFromVisual(a.visual);

		// find physical device the window is presented on
		// (there should be only one compatible device)
		vector<vk::PhysicalDevice> deviceList=instance->enumeratePhysicalDevices();
		vector<string> compatibleDevices;
		for(vk::PhysicalDevice pd:deviceList) {
			uint32_t c;
			pd.getQueueFamilyProperties(&c,nullptr);
			for(uint32_t i=0; i<c; i++)
				if(pd.getXlibPresentationSupportKHR(i,d,v)) {
					compatibleDevices.push_back(pd.getProperties().deviceName);
					break;
				}
		}
		if(compatibleDevices.size()==1)
			cout<<"Active device: "<<compatibleDevices[0]<<endl;
		else if(compatibleDevices.size()==0)
			cout<<"Warning: no compatible devices."<<endl;
		else {
			auto it=compatibleDevices.cbegin();
			cout<<"Warning: more compatible devices ("<<*it;
			for(it++; it!=compatibleDevices.end(); it++)
				cout<<", "<<*it;
			cout<<")"<<endl;
		}

		// run event loop
		while(true) {
			XEvent e;
			XNextEvent(d,&e);
			if(e.type==ClientMessage&&ulong(e.xclient.data.l[0])==wmDeleteMessage)
				break;
		}

	// catch exceptions
	} catch(vk::Error &e) {
		cout<<"Failed because of Vulkan exception: "<<e.what()<<endl;
	} catch(exception &e) {
		cout<<"Failed because of exception: "<<e.what()<<endl;
	} catch(...) {
		cout<<"Failed because of unspecified exception."<<endl;
	}

	// clean up
	if(d)
		XCloseDisplay(d);

	return 0;
}
