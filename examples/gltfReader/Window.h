#pragma once

#ifdef _WIN32
# define VK_USE_PLATFORM_WIN32_KHR
#else
# include <X11/Xlib.h>
# define VK_USE_PLATFORM_XLIB_KHR
#endif
#include <vulkan/vulkan.hpp>

// forward declarations
struct VkXlibSurfaceCreateInfoKHR;
typedef VkResult (VKAPI_PTR *PFN_vkCreateXlibSurfaceKHR)(VkInstance instance, const VkXlibSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);

namespace CadR {
	class VulkanInstance;
}

namespace CadUI {


class Window final {
protected:
#ifdef _WIN32
	HWND window=nullptr;
#else
	Display* display=nullptr;
	::Window window=0;
	Atom wmDeleteMessage;
#endif
	vk::Extent2D currentSurfaceExtent = vk::Extent2D(0,0);
	vk::Extent2D windowSize;
	bool needResize=true;

	CadR::VulkanInstance* _instance;
	vk::SurfaceKHR _surface;

public:

	~Window();
	Window(CadR::VulkanInstance& instance);
	void init(CadR::VulkanInstance& instance);

	vk::SurfaceKHR surface() const;

	PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
	PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;

};


// inline methods
inline Window::Window(CadR::VulkanInstance& instance)  { init(instance); }
inline vk::SurfaceKHR Window::surface() const  { return _surface; }


}
