#pragma once

#include <CadR/CallbackList.h>
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
	vk::Extent2D _surfaceExtent = vk::Extent2D(0,0);
	vk::Extent2D _windowSize;
	bool _needResize = true;

	CadR::VulkanInstance* _instance;
	CadR::VulkanDevice* _device;
	vk::PhysicalDevice _physicalDevice;
	vk::SurfaceFormatKHR _surfaceFormat;
	uint32_t _graphicsQueueFamily;
	uint32_t _presentationQueueFamily;
	vk::RenderPass _renderPass;
	vk::PresentModeKHR _presentMode;
	vk::SurfaceKHR _surface;
	vk::SwapchainKHR _swapchain;
	std::vector<vk::ImageView> _swapchainImageViews;
	std::vector<vk::Framebuffer> _framebuffers;

public:

	Window() = delete;
	Window(CadR::VulkanInstance& instance);
	Window(Window&& other);
	~Window();

	Window& operator=(Window&& rhs);

	void initVulkan(vk::PhysicalDevice physicalDevice,CadR::VulkanDevice& device,vk::SurfaceFormatKHR surfaceFormat,
	                uint32_t graphicsQueueFamily,uint32_t presentationQueueFamily,vk::RenderPass renderPass,
	                vk::PresentModeKHR presentMode=vk::PresentModeKHR::eFifo); // eFifo is guaranteed to be supported everywhere
	void cleanUpVulkan();

	bool processEvents();
	bool updateSize();
	CadR::CallbackList<void()> resizeCallbacks;

	void recreateSwapchain();
	void recreateSwapchain(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities);

	void scheduleNeedResize();
	std::tuple<uint32_t,bool> acquireNextImage(vk::Semaphore semaphoreToSignal,vk::Fence fenceToSignal=vk::Fence());
	bool present(vk::Queue presentationQueue,vk::Semaphore semaphoreToWait,uint32_t imageIndex);

	CadR::VulkanInstance* vulkanInstance() const;
	CadR::VulkanDevice* device() const;
	vk::PhysicalDevice physicalDevice() const;
	vk::Extent2D surfaceExtent() const;
	vk::SurfaceFormatKHR surfaceFormat() const;
	uint32_t graphicsQueueFamily() const;
	uint32_t presentationQueueFamily() const;
	vk::RenderPass renderPass() const;
	vk::PresentModeKHR presentMode() const;
	vk::SurfaceKHR surface() const;
	vk::SwapchainKHR swapchain() const;
	uint32_t imageCount() const;
	const std::vector<vk::ImageView> swapchainImageViews() const;
	const std::vector<vk::Framebuffer>& framebuffers() const;

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
	PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
	PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
	PFN_vkQueuePresentKHR vkQueuePresentKHR;

protected:
	Window(const Window& other) = default;  ///< Private copy contructor. Object copies not allowed. Only internal use.
	Window& operator=(const Window&) = default;  ///< Private copy assignment. Object copies not allowed. Only internal use.
	void create(CadR::VulkanInstance& instance);
	void destroy();
};


// inline methods
inline Window::Window(CadR::VulkanInstance& instance) : _instance(nullptr), _device(nullptr)  { create(instance); }
inline void Window::scheduleNeedResize()  { _needResize=true; }
inline CadR::VulkanInstance* Window::vulkanInstance() const  { return _instance; }
inline CadR::VulkanDevice* Window::device() const  { return _device; }
inline vk::PhysicalDevice Window::physicalDevice() const  { return _physicalDevice; }
inline vk::Extent2D Window::surfaceExtent() const  { return _surfaceExtent; }
inline vk::SurfaceFormatKHR Window::surfaceFormat() const  { return _surfaceFormat; }
inline uint32_t Window::graphicsQueueFamily() const  { return _graphicsQueueFamily; }
inline uint32_t Window::presentationQueueFamily() const  { return _presentationQueueFamily; }
inline vk::RenderPass Window::renderPass() const  { return _renderPass; }
inline vk::PresentModeKHR Window::presentMode() const  { return _presentMode; }
inline vk::SurfaceKHR Window::surface() const  { return _surface; }
inline vk::SwapchainKHR Window::swapchain() const  { return _swapchain; }
inline uint32_t Window::imageCount() const  { return uint32_t(_framebuffers.size()); }
inline const std::vector<vk::ImageView> Window::swapchainImageViews() const  { return _swapchainImageViews; }
inline const std::vector<vk::Framebuffer>& Window::framebuffers() const  { return _framebuffers; }


}
