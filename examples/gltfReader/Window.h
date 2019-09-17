#pragma once

#ifdef _WIN32
# include <windows.h>
# define VK_USE_PLATFORM_WIN32_KHR
#else
# include <X11/Xlib.h>
# define VK_USE_PLATFORM_XLIB_KHR
#endif
#include <vulkan/vulkan.hpp>

// forward declarations
struct VkXlibSurfaceCreateInfoKHR;
struct VkWin32SurfaceCreateInfoKHR;
typedef VkResult (VKAPI_PTR *PFN_vkCreateXlibSurfaceKHR)(VkInstance instance, const VkXlibSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
typedef VkResult (VKAPI_PTR *PFN_vkCreateWin32SurfaceKHR)(VkInstance instance, const VkWin32SurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);

namespace CadR {
	class VulkanDevice;
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
	vk::Extent2D _currentSurfaceExtent = vk::Extent2D(0,0);
	vk::Extent2D _windowSize;
	bool _needResize = true;

	CadR::VulkanInstance* _instance;
	CadR::VulkanDevice* _device;
	vk::PhysicalDevice _physicalDevice;
	vk::SurfaceFormatKHR _surfaceFormat;
	uint32_t _graphicsQueueFamily;
	uint32_t _presentationQueueFamily;
	vk::PresentModeKHR _presentMode;
	vk::SurfaceKHR _surface;
	vk::SwapchainKHR _swapchain;

public:

	Window() = delete;
	Window(CadR::VulkanInstance& instance);
	Window(Window&& other);
	~Window();

	Window& operator=(Window&& rhs);

	void initVulkan(vk::PhysicalDevice physicalDevice,CadR::VulkanDevice& device,vk::SurfaceFormatKHR surfaceFormat,
	           uint32_t graphicsQueueFamily,uint32_t presentationQueueFamily,
	           vk::PresentModeKHR presentMode=vk::PresentModeKHR::eFifo); // eFifo is guaranteed to be supported everywhere
	void cleanUpVulkan();

	bool processEvents();
	bool updateSize();

	void recreateSwapchain();

	vk::SurfaceKHR surface() const;

#ifdef _WIN32
	PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
#else
	PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
#endif
	PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
	PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
	PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;

protected:
	Window(const Window& other) = default;  ///< Private copy contructor. Object copies not allowed. Only internal use.
	Window& operator=(const Window&) = default;  ///< Private copy assignment. Object copies not allowed. Only internal use.
	void create(CadR::VulkanInstance& instance);
	void destroy();
};


// inline methods
inline Window::Window(CadR::VulkanInstance& instance) : _instance(nullptr), _device(nullptr)  { create(instance); }
inline vk::SurfaceKHR Window::surface() const  { return _surface; }


}
