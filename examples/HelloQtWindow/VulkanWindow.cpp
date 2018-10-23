#include "VulkanWindow.h"
#include <QX11Info>



void VulkanWindow::showEvent(QShowEvent* e)
{
	inherited::showEvent(e);

	if(!_surface) {

		// create surface
		vk::Instance instance(VkInstance(intptr_t(vulkanInstance()->vkInstance())));
#ifdef _WIN32
		_surface=instance.createWin32SurfaceKHRUnique(vk::Win32SurfaceCreateInfoKHR(vk::Win32SurfaceCreateFlagsKHR(),wc.hInstance,window));
#else
		_surface=instance.createXlibSurfaceKHRUnique(vk::XlibSurfaceCreateInfoKHR(vk::XlibSurfaceCreateFlagsKHR(),QX11Info::display(),winId()));
#endif
		throw std::string("Got it");

	}
}
