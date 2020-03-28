#pragma once

#include <vulkan/vulkan.hpp>
#include <tuple>

namespace CadR {

class VulkanLibrary;


class CADR_EXPORT VulkanInstance final {
protected:
	vk::Instance _instance;
public:

	std::tuple<vk::PhysicalDevice,uint32_t,uint32_t> chooseDeviceAndQueueFamilies(vk::SurfaceKHR surface);

	VulkanInstance();
	VulkanInstance(VulkanLibrary& lib,const vk::InstanceCreateInfo& createInfo);
	VulkanInstance(VulkanLibrary& lib,vk::Instance instance);
	VulkanInstance(VulkanLibrary& lib,
	               const char* applicationName = nullptr,
	               uint32_t applicationVersion = 0,
	               const char* engineName = nullptr,
	               uint32_t engineVersion = 0,
	               uint32_t apiVersion = 0,
	               vk::ArrayProxy<const char*const> enabledLayers = nullptr,
	               vk::ArrayProxy<const char*const> enabledExtensions = nullptr);
	~VulkanInstance();

	void create(VulkanLibrary& lib,const vk::InstanceCreateInfo& createInfo);
	void create(VulkanLibrary& lib,
	            const char* applicationName = nullptr,
	            uint32_t applicationVersion = 0,
	            const char* engineName = nullptr,
	            uint32_t engineVersion = 0,
	            uint32_t apiVersion = 0,
	            vk::ArrayProxy<const char*const> enabledLayers = nullptr,
	            vk::ArrayProxy<const char*const> enabledExtensions = nullptr);
	void init(VulkanLibrary& lib,vk::Instance instance);
	bool initialized() const;
	void destroy();

	VulkanInstance(VulkanInstance&& other) noexcept;
	VulkanInstance& operator=(VulkanInstance&& rhs) noexcept;

	template<typename T> T getProcAddr(const char* name) const;
	template<typename T> T getProcAddr(const std::string& name) const;

	operator vk::Instance() const;
	explicit operator bool() const;
	bool operator!() const;

	vk::Instance get() const;
	void set(nullptr_t);

	inline PFN_vkVoidFunction getProcAddr(const char* pName) const  { return _instance.getProcAddr(pName,*this); }
	inline void destroy(const vk::AllocationCallbacks* pAllocator) const noexcept  { _instance.destroy(pAllocator,*this); }
	inline vk::Result createDevice(vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo* pCreateInfo,const vk::AllocationCallbacks* pAllocator,vk::Device* pDevice) const  { return physicalDevice.createDevice(pCreateInfo,pAllocator,pDevice,*this); }
	inline vk::Result enumeratePhysicalDevices(uint32_t* pPhysicalDeviceCount,vk::PhysicalDevice* pPhysicalDevices) const  { return _instance.enumeratePhysicalDevices(pPhysicalDeviceCount,pPhysicalDevices,*this); }
	inline void getPhysicalDeviceProperties(vk::PhysicalDevice physicalDevice,vk::PhysicalDeviceProperties* pProperties) const noexcept  { physicalDevice.getProperties(pProperties,*this); }
	inline vk::Result enumerateDeviceExtensionProperties(vk::PhysicalDevice physicalDevice,const char* pLayerName,uint32_t* pPropertyCount,vk::ExtensionProperties* pProperties) const  { return physicalDevice.enumerateDeviceExtensionProperties(pLayerName,pPropertyCount,pProperties,*this); }
	inline void getPhysicalDeviceFormatProperties(vk::PhysicalDevice physicalDevice,vk::Format format,vk::FormatProperties* pFormatProperties) const noexcept  { physicalDevice.getFormatProperties(format,pFormatProperties,*this); }
	inline void getPhysicalDeviceMemoryProperties(vk::PhysicalDevice physicalDevice,vk::PhysicalDeviceMemoryProperties* pMemoryProperties) const noexcept  { physicalDevice.getMemoryProperties(pMemoryProperties,*this); }
	inline void getPhysicalDeviceQueueFamilyProperties(vk::PhysicalDevice physicalDevice,uint32_t* pQueueFamilyPropertyCount,vk::QueueFamilyProperties* pQueueFamilyProperties) const noexcept  { physicalDevice.getQueueFamilyProperties(pQueueFamilyPropertyCount,pQueueFamilyProperties,*this); }

#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
	inline void destroy(vk::Optional<const vk::AllocationCallbacks> allocator) const noexcept  { _instance.destroy(allocator,*this); }
	inline vk::ResultValueType<vk::Device>::type createDevice(vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return physicalDevice.createDevice(createInfo,allocator,*this); }
	template<typename Allocator=std::allocator<vk::PhysicalDevice>>
	typename vk::ResultValueType<std::vector<vk::PhysicalDevice,Allocator>>::type enumeratePhysicalDevices() const  { return _instance.enumeratePhysicalDevices(*this); }
	template<typename Allocator=std::allocator<vk::PhysicalDevice>>
	typename vk::ResultValueType<std::vector<vk::PhysicalDevice,Allocator>>::type enumeratePhysicalDevices(Allocator const& vectorAllocator) const  { return _instance.enumeratePhysicalDevices(vectorAllocator,*this); }
	inline vk::PhysicalDeviceProperties getPhysicalDeviceProperties(vk::PhysicalDevice physicalDevice) const noexcept  { return physicalDevice.getProperties(*this); }
	template<typename Allocator=std::allocator<vk::ExtensionProperties>>
	typename vk::ResultValueType<std::vector<vk::ExtensionProperties,Allocator>>::type enumerateDeviceExtensionProperties(vk::PhysicalDevice physicalDevice,vk::Optional<const std::string> layerName=nullptr) const  { return physicalDevice.enumerateDeviceExtensionProperties(layerName,*this); }
	template<typename Allocator=std::allocator<vk::ExtensionProperties>>
	typename vk::ResultValueType<std::vector<vk::ExtensionProperties,Allocator>>::type enumerateDeviceExtensionProperties(vk::PhysicalDevice physicalDevice,vk::Optional<const std::string> layerName,Allocator const& vectorAllocator) const  { return physicalDevice.enumerateDeviceExtensionProperties(layerName,vectorAllocator,*this); }
	inline vk::FormatProperties getPhysicalDeviceFormatProperties(vk::PhysicalDevice physicalDevice,vk::Format format) const noexcept  { return physicalDevice.getFormatProperties(format,*this); }
	inline vk::PhysicalDeviceMemoryProperties getPhysicalDeviceMemoryProperties(vk::PhysicalDevice physicalDevice) const noexcept  { return physicalDevice.getMemoryProperties(*this); }
	template<typename Allocator=std::allocator<vk::QueueFamilyProperties>>
	std::vector<vk::QueueFamilyProperties,Allocator> getPhysicalDeviceQueueFamilyProperties(vk::PhysicalDevice physicalDevice) const  { return physicalDevice.getQueueFamilyProperties(*this); }
	template<typename Allocator=std::allocator<vk::QueueFamilyProperties>>
	std::vector<vk::QueueFamilyProperties,Allocator> getPhysicalDeviceQueueFamilyProperties(vk::PhysicalDevice physicalDevice,Allocator const& vectorAllocator) const  { return physicalDevice.getQueueFamilyProperties(vectorAllocator,*this); }
#endif

#ifndef VULKAN_HPP_NO_SMART_HANDLE
	template<typename Dispatch>
	typename vk::ResultValueType<vk::UniqueHandle<vk::Device,Dispatch>>::type createDeviceUnique(vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator,Dispatch const &d) const  { return physicalDevice.createDeviceUnique(createInfo,allocator,d); }
#endif

	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
	PFN_vkDestroyInstance vkDestroyInstance;
	PFN_vkCreateDevice vkCreateDevice;
	PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
	PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
	PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
	PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
	PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties;
	PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
	PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;

private:
	VulkanInstance(const VulkanInstance&) = default;  ///< Private copy contructor. Object copies not allowed. Only internal use.
	VulkanInstance& operator=(const VulkanInstance&) = default;  ///< Private copy assignment. Object copies not allowed. Only internal use.
};


// inline and template methods
inline VulkanInstance::VulkanInstance()  { vkDestroyInstance=nullptr; vkCreateDevice=nullptr; vkGetDeviceProcAddr=nullptr; }
inline VulkanInstance::VulkanInstance(VulkanLibrary& lib,const vk::InstanceCreateInfo& createInfo)  { create(lib,createInfo); }
inline VulkanInstance::VulkanInstance(VulkanLibrary& lib,vk::Instance instance)  { init(lib,instance); }
inline VulkanInstance::VulkanInstance(VulkanLibrary& lib,const char* applicationName,uint32_t applicationVersion,
		const char* engineName,uint32_t engineVersion,uint32_t apiVersion,
		vk::ArrayProxy<const char*const> enabledLayers,vk::ArrayProxy<const char*const> enabledExtensions)
{
	vk::ApplicationInfo appInfo(applicationName,applicationVersion,engineName,engineVersion,apiVersion);
	vk::InstanceCreateInfo createInfo(vk::InstanceCreateFlags(),&appInfo,enabledLayers.size(),enabledLayers.data(),enabledExtensions.size(),enabledExtensions.data());
	create(lib,createInfo);
}
inline VulkanInstance::~VulkanInstance()  { destroy(); }

inline void VulkanInstance::create(VulkanLibrary& lib,const char* applicationName,uint32_t applicationVersion,
		const char* engineName,uint32_t engineVersion,uint32_t apiVersion,
		vk::ArrayProxy<const char*const> enabledLayers,vk::ArrayProxy<const char*const> enabledExtensions)
{
	vk::ApplicationInfo appInfo(applicationName,applicationVersion,engineName,engineVersion,apiVersion);
	vk::InstanceCreateInfo createInfo(vk::InstanceCreateFlags(),&appInfo,enabledLayers.size(),enabledLayers.data(),enabledExtensions.size(),enabledExtensions.data());
	create(lib,createInfo);
}
inline bool VulkanInstance::initialized() const  { return _instance.operator bool(); }
template<typename T> T VulkanInstance::getProcAddr(const char* name) const  { return reinterpret_cast<T>(_instance.getProcAddr(name,*this)); }
template<typename T> T VulkanInstance::getProcAddr(const std::string& name) const  { return reinterpret_cast<T>(_instance.getProcAddr(name,*this)); }

inline VulkanInstance::operator vk::Instance() const  { return _instance; }
inline VulkanInstance::operator bool() const  { return _instance.operator bool(); }
inline bool VulkanInstance::operator!() const  { return _instance.operator!(); }
inline vk::Instance VulkanInstance::get() const  { return _instance; }
inline void VulkanInstance::set(nullptr_t)  { _instance=nullptr; }


}
