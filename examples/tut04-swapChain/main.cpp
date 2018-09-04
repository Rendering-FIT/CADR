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


int main(int,char**)
{
#ifdef _WIN32
	HWND w=nullptr;
#else
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
const char *info_instance_extensions[] = {VK_EXT_DISPLAY_SURFACE_COUNTER_EXTENSION_NAME,
                                              VK_KHR_DISPLAY_EXTENSION_NAME,
                                              VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
                                              VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
                                              //VK_KHR_SHARED_PRESENTABLE_IMAGE_EXTENSION_NAME,
                                              VK_KHR_SURFACE_EXTENSION_NAME,
//VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME,
VK_KHR_XLIB_SURFACE_EXTENSION_NAME };
		vk::UniqueHandle<vk::Instance> instance(
			vk::createInstance(
				vk::InstanceCreateInfo{
					vk::InstanceCreateFlags(),  // flags
					&(const vk::ApplicationInfo&)vk::ApplicationInfo{
						"CADR tut04",            // application name
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
					6,        // enabled extension count
					//array<const char*,2>{{"VK_KHR_surface","VK_KHR_xlib_surface"}}.data(),  // enabled extension names
					info_instance_extensions
				}));

#ifdef _WIN32
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
		vector<vk::PhysicalDevice> compatibleDevices;
		for(vk::PhysicalDevice pd:deviceList) {
			uint32_t c;
			pd.getQueueFamilyProperties(&c,nullptr);
			for(uint32_t i=0; i<c; i++)
				if(pd.getSurfaceSupportKHR(i,s.get())) {
					auto extensionList=pd.enumerateDeviceExtensionProperties();
					for(vk::ExtensionProperties& e:extensionList)
						if(strcmp(e.extensionName,"VK_KHR_swapchain")==0) {
							compatibleDevices.push_back(pd);
							break;
						}
				}
		}
		cout<<"Compatible devices:"<<endl;
		for(vk::PhysicalDevice& pd:compatibleDevices)
			cout<<"   "<<pd.getProperties().deviceName<<endl;

		for(vk::PhysicalDevice& pd:compatibleDevices) {
			std::vector<vk::SurfaceFormatKHR> surfaceFormats;
			uint32_t surfaceFormatCount=1;
			vk::DispatchLoaderDynamic dld(instance.get());
			vk::Result result;
			result = static_cast<vk::Result>( dld.vkGetPhysicalDeviceSurfaceFormatsKHR( pd, s.get(), &surfaceFormatCount, nullptr ) );
#if 0
-   //do {
		 surfaceFormatCount*=2;
      //result = static_cast<vk::Result>( vkGetPhysicalDeviceSurfaceFormatsKHR( pd, s.get(), &surfaceFormatCount, nullptr ) );
      //if ( ( result == vk::Result::eSuccess ) && surfaceFormatCount )
      {
        surfaceFormats.resize( surfaceFormatCount );
        result = static_cast<vk::Result>( vkGetPhysicalDeviceSurfaceFormatsKHR( pd, s.get(), &surfaceFormatCount, reinterpret_cast<VkSurfaceFormatKHR*>( surfaceFormats.data() ) ) );
      }
    //} while ( 0);//result == vk::Result::eIncomplete );
    /*VULKAN_HPP_ASSERT( surfaceFormatCount <= surfaceFormats.size() );
    surfaceFormats.resize( surfaceFormatCount );
    auto formatList=createResultValue( result, surfaceFormats, VULKAN_HPP_NAMESPACE_STRING"::PhysicalDevice::getSurfaceFormatsKHR" );*/
			//auto formatList=pd.getSurfaceFormatsKHR(s.get());
			//for(auto f:formatList)
			//	cout<<vk::to_string(f)<<endl;
			/*auto presentModeList=pd.getSurfacePresentModesKHR(s.get());
			for(auto p:presentModeList)
				cout<<vk::to_string(p)<<endl;*/
#endif
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
	if(w)
		XDestroyWindow(d,w);
	if(d)
		XCloseDisplay(d);

	return 0;
}
