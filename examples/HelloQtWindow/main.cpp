#include <QGuiApplication>
#include <QScreen>
#include "VulkanWindow.h"
#include <vulkan/vulkan.hpp>
#include <iostream>

using namespace std;


// Vulkan instance
// (must be destructed as the last one, at least on Linux, it must be destroyed after display connection)
static vk::UniqueInstance instance;
static QVulkanInstance qtVulkanInstance;


/// Init Vulkan and open the window.
static void init()
{
#if 0
#ifdef _WIN32

	// initial window size
	RECT screenSize;
	if(GetWindowRect(GetDesktopWindow(),&screenSize)==0)
		throw runtime_error("GetWindowRect() failed.");
	windowWidth=(screenSize.right-screenSize.left)/2;
	windowHeight=(screenSize.bottom-screenSize.top)/2;


#else


	// create window
	Screen* screen=XDefaultScreenOfDisplay(display);
	uint32_t windowWidth=XWidthOfScreen(screen)/2;
	uint32_t windowHeight=XHeightOfScreen(screen)/2;
	window=XCreateSimpleWindow(display,DefaultRootWindow(display),0,0,windowWidth,
		                        windowHeight,0,blackColor,blackColor);
	XSetStandardProperties(display,window,"Hello window!","Hello window!",None,NULL,0,NULL);

#endif

#endif
}


/// main function of the application
int main(int argc,char** argv)
{
	// catch exceptions
	// (vulkan.hpp fuctions throws if they fail)
	int r=1;
	try {

		// init Qt
		QGuiApplication qtApp(argc,argv);

		// Vulkan instance
		instance=
			vk::createInstanceUnique(
				vk::InstanceCreateInfo{
					vk::InstanceCreateFlags(),  // flags
					&(const vk::ApplicationInfo&)vk::ApplicationInfo{
						"CADR Vk HelloQtWindow", // application name
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
		qtVulkanInstance.setVkInstance(instance.get());

		// open window
		VulkanWindow window;
		window.setVulkanInstance(&qtVulkanInstance);
		//QScreen* screen=window.screen();
		//window.resize(screen?screen->size()/2:QSize(400,300));
		window.show();
		//init();

#if 0
			// queue frame
			if(!queueFrame())
				goto ExitMainLoop;

			// wait for rendering to complete
			presentationQueue.waitIdle();
		}
#endif
		r=qtApp.exec();
//	ExitMainLoop:;
//		device->waitIdle();

	// catch exceptions
	} catch(vk::Error &e) {
		cout<<"Failed because of Vulkan exception: "<<e.what()<<endl;
	} catch(exception &e) {
		cout<<"Failed because of exception: "<<e.what()<<endl;
	} catch(...) {
		cout<<"Failed because of unspecified exception."<<endl;
	}

	return r;
}
