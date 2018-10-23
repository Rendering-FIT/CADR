#include "VulkanWindow.h"
#ifndef _WIN32
# include <QX11Info>
#endif



void VulkanWindow::showEvent(QShowEvent* e)
{
	inherited::showEvent(e);

	if(!_surface) {

		// create surface
		vk::Instance instance(vulkanInstance()->vkInstance());
#ifdef _WIN32
		_surface=instance.createWin32SurfaceKHRUnique(vk::Win32SurfaceCreateInfoKHR(vk::Win32SurfaceCreateFlagsKHR(),GetModuleHandle(NULL),(HWND)winId()));
#else
		_surface=instance.createXlibSurfaceKHRUnique(vk::XlibSurfaceCreateInfoKHR(vk::XlibSurfaceCreateFlagsKHR(),QX11Info::display(),winId()));
#endif

	}
}
