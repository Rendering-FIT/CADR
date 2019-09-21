#pragma once

#include <tuple>
#include <vulkan/vulkan.hpp>
#include <CadR/CallbackList.h>
#include <CadR/Export.h>

namespace CadR {

class VulkanInstance;


class CADR_EXPORT VulkanDevice final {
protected:
	vk::Device _device;
public:

	VulkanDevice();
	VulkanDevice(VulkanInstance& instance,vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo& createInfo);
	VulkanDevice(VulkanDevice&& vd) noexcept;
	VulkanDevice(VulkanInstance& instance,
	             std::tuple<vk::PhysicalDevice,uint32_t,uint32_t> physicalDeviceAndQueueFamilies,
	             const vk::ArrayProxy<const char*const> enabledLayers = nullptr,
	             const vk::ArrayProxy<const char*const> enabledExtensions = nullptr,
	             const vk::PhysicalDeviceFeatures* enabledFeatures = nullptr);
	~VulkanDevice();

	VulkanDevice& operator=(VulkanDevice&& rhs) noexcept;

	void init(VulkanInstance& instance,vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo& createInfo);
	void init(VulkanInstance& instance,
	          std::tuple<vk::PhysicalDevice,uint32_t,uint32_t> physicalDeviceAndQueueFamilies,
	          const vk::ArrayProxy<const char*const> enabledLayers = nullptr,
	          const vk::ArrayProxy<const char*const> enabledExtensions = nullptr,
	          const vk::PhysicalDeviceFeatures* enabledFeatures = nullptr);
	void cleanUp();

	CallbackList<void()> cleanUpCallbacks;

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
	PFN_vkGetDeviceQueue vkGetDeviceQueue;
	PFN_vkCreateRenderPass vkCreateRenderPass;
	PFN_vkDestroyRenderPass vkDestroyRenderPass;
	PFN_vkCreateBuffer vkCreateBuffer;
	PFN_vkDestroyBuffer vkDestroyBuffer;
	PFN_vkAllocateMemory vkAllocateMemory;
	PFN_vkBindBufferMemory vkBindBufferMemory;
	PFN_vkFreeMemory vkFreeMemory;
	PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
	PFN_vkMapMemory vkMapMemory;
	PFN_vkUnmapMemory vkUnmapMemory;
	PFN_vkCreateImageView vkCreateImageView;
	PFN_vkDestroyImageView vkDestroyImageView;
	PFN_vkCreateFramebuffer vkCreateFramebuffer;
	PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
	PFN_vkCreateCommandPool vkCreateCommandPool;
	PFN_vkDestroyCommandPool vkDestroyCommandPool;
	PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
	PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
	PFN_vkEndCommandBuffer vkEndCommandBuffer;
	PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
	PFN_vkCreateFence vkCreateFence;
	PFN_vkDestroyFence vkDestroyFence;
	PFN_vkQueueSubmit vkQueueSubmit;
	PFN_vkWaitForFences vkWaitForFences;

private:
	VulkanDevice(const VulkanDevice&) = default;  ///< Private copy contructor. Object copies not allowed. Only internal use.
	VulkanDevice& operator=(const VulkanDevice&) = default;  ///< Private copy assignment. Object copies not allowed. Only internal use.
};


// inline methods
inline VulkanDevice::VulkanDevice()  { vkGetDeviceProcAddr=nullptr; vkDestroyDevice=nullptr; }
inline VulkanDevice::VulkanDevice(VulkanInstance& instance,vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo& createInfo)  { init(instance,physicalDevice,createInfo); }
inline VulkanDevice::~VulkanDevice()  { cleanUp(); }

inline VulkanDevice::VulkanDevice(VulkanInstance& instance,std::tuple<vk::PhysicalDevice,uint32_t,uint32_t> physicalDeviceAndQueueFamilies,
		const vk::ArrayProxy<const char*const> enabledLayers,const vk::ArrayProxy<const char*const> enabledExtensions,const vk::PhysicalDeviceFeatures* enabledFeatures)
	{ init(instance,physicalDeviceAndQueueFamilies,enabledLayers,enabledExtensions,enabledFeatures); }
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
