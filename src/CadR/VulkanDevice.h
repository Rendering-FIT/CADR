#pragma once

#include <vulkan/vulkan.hpp>

namespace CadR {

class VulkanInstance;


class VulkanDevice final {
protected:
	vk::Device _device;
public:

	VulkanDevice();
	VulkanDevice(VulkanInstance& instance,vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo& createInfo);
	VulkanDevice(VulkanInstance& instance,vk::PhysicalDevice physicalDevice,
	             const vk::ArrayProxy<const vk::DeviceQueueCreateInfo> queueInfos,
	             const vk::ArrayProxy<const char*> enabledLayers = nullptr,
	             const vk::ArrayProxy<const char*> enabledExtensions = nullptr,
	             const vk::PhysicalDeviceFeatures* enabledFeatures = nullptr);
	~VulkanDevice();

	void init(VulkanInstance& instance,vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo& createInfo);
	void init(VulkanInstance& instance,vk::PhysicalDevice physicalDevice,
	          const vk::ArrayProxy<const vk::DeviceQueueCreateInfo> queueInfos,
	          const vk::ArrayProxy<const char*> enabledLayers = nullptr,
	          const vk::ArrayProxy<const char*> enabledExtensions = nullptr,
	          const vk::PhysicalDeviceFeatures* enabledFeatures = nullptr);
	void reset();

	VulkanDevice(VulkanDevice&&);
	VulkanDevice& operator=(VulkanDevice&&);

	template<typename T> T getProcAddr(const char* name) const;
	template<typename T> T getProcAddr(const std::string& name) const;

	operator vk::Device() const;
	explicit operator bool() const;
	bool operator!() const;

	const vk::Device* operator->() const;
	vk::Device* operator->();
	const vk::Device& operator*() const;
	vk::Device& operator*();
	const vk::Device& get() const;
	vk::Device& get();

	PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
	PFN_vkDestroyDevice vkDestroyDevice;
	PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
	PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool;

private:
	VulkanDevice(const VulkanDevice&) = default;  ///< Private copy contructor. Object copies not allowed. Only internal use.
	VulkanDevice& operator=(const VulkanDevice&) = default;  ///< Private copy assignment. Object copies not allowed. Only internal use.
};


// inline methods
inline VulkanDevice::VulkanDevice()  { vkGetDeviceProcAddr=nullptr; vkDestroyDevice=nullptr; }
inline VulkanDevice::VulkanDevice(VulkanInstance& instance,vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo& createInfo)  { init(instance,physicalDevice,createInfo); }
inline VulkanDevice::VulkanDevice(VulkanInstance& instance,vk::PhysicalDevice physicalDevice,
		const vk::ArrayProxy<const vk::DeviceQueueCreateInfo> queueInfos,const vk::ArrayProxy<const char*> enabledLayers,
		const vk::ArrayProxy<const char*> enabledExtensions,const vk::PhysicalDeviceFeatures* enabledFeatures)
	{	vk::DeviceCreateInfo createInfo(vk::DeviceCreateFlags(),queueInfos.size(),queueInfos.data(),enabledLayers.size(),enabledLayers.data(),enabledExtensions.size(),enabledExtensions.data(),enabledFeatures);
		init(instance,physicalDevice,createInfo); }
inline VulkanDevice::~VulkanDevice()  { reset(); }

inline void VulkanDevice::init(VulkanInstance& instance,vk::PhysicalDevice physicalDevice,
		const vk::ArrayProxy<const vk::DeviceQueueCreateInfo> queueInfos,const vk::ArrayProxy<const char*> enabledLayers,
		const vk::ArrayProxy<const char*> enabledExtensions,const vk::PhysicalDeviceFeatures* enabledFeatures)
	{	vk::DeviceCreateInfo createInfo(vk::DeviceCreateFlags(),queueInfos.size(),queueInfos.data(),enabledLayers.size(),enabledLayers.data(),enabledExtensions.size(),enabledExtensions.data(),enabledFeatures);
		init(instance,physicalDevice,createInfo); }
template<typename T> T VulkanDevice::getProcAddr(const char* name) const  { return reinterpret_cast<T>(_device.getProcAddr(name,*this)); }
template<typename T> T VulkanDevice::getProcAddr(const std::string& name) const  { return reinterpret_cast<T>(_device.getProcAddr(name,*this)); }

inline VulkanDevice::operator vk::Device() const  { return _device; }
inline VulkanDevice::operator bool() const  { return _device.operator bool(); }
inline bool VulkanDevice::operator!() const  { return _device.operator!(); }
inline const vk::Device* VulkanDevice::operator->() const  { return &_device; }
inline vk::Device* VulkanDevice::operator->()  { return &_device; }
inline const vk::Device& VulkanDevice::operator*() const  { return _device; }
inline vk::Device& VulkanDevice::operator*()  { return _device; }
inline const vk::Device& VulkanDevice::get() const  { return _device; }
inline vk::Device& VulkanDevice::get()  { return _device; }


}
