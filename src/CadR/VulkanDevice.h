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

	inline PFN_vkVoidFunction getProcAddr(const char* pName) const  { return _device.getProcAddr(pName,*this); }
	inline void destroy(const vk::AllocationCallbacks* pAllocator) const  { _device.destroy(pAllocator,*this); }
	inline void getQueue(uint32_t queueFamilyIndex,uint32_t queueIndex,vk::Queue* pQueue) const  { _device.getQueue(queueFamilyIndex,queueIndex,pQueue,*this); }
	inline vk::Result createRenderPass(const vk::RenderPassCreateInfo* pCreateInfo,const vk::AllocationCallbacks* pAllocator,vk::RenderPass* pRenderPass) const  { return _device.createRenderPass(pCreateInfo,pAllocator,pRenderPass,*this); }
	inline void destroyRenderPass(vk::RenderPass renderPass,const vk::AllocationCallbacks* pAllocator) const  { _device.destroyRenderPass(renderPass,pAllocator,*this); }
	inline void destroy(vk::RenderPass renderPass,const vk::AllocationCallbacks* pAllocator) const  { _device.destroyRenderPass(renderPass,pAllocator,*this); }
	inline vk::Result createBuffer(const vk::BufferCreateInfo* pCreateInfo,const vk::AllocationCallbacks* pAllocator,vk::Buffer* pBuffer) const  { return _device.createBuffer(pCreateInfo,pAllocator,pBuffer); }
	inline void destroyBuffer(vk::Buffer buffer,const vk::AllocationCallbacks* pAllocator) const  { _device.destroyBuffer(buffer,pAllocator,*this); }
	inline void destroy(vk::Buffer buffer,const vk::AllocationCallbacks* pAllocator) const  { _device.destroyBuffer(buffer,pAllocator,*this); }
	inline vk::Result allocateMemory(const vk::MemoryAllocateInfo* pAllocateInfo,const vk::AllocationCallbacks* pAllocator,vk::DeviceMemory* pMemory) const  { return _device.allocateMemory(pAllocateInfo,pAllocator,pMemory,*this); }
    inline void freeMemory(vk::DeviceMemory memory,const vk::AllocationCallbacks* pAllocator) const  { _device.freeMemory(memory,pAllocator,*this); }
	inline void getBufferMemoryRequirements(vk::Buffer buffer,vk::MemoryRequirements* pMemoryRequirements) const  { _device.getBufferMemoryRequirements(buffer,pMemoryRequirements,*this); }
	inline vk::Result mapMemory(vk::DeviceMemory memory,vk::DeviceSize offset,vk::DeviceSize size,vk::MemoryMapFlags flags,void** ppData) const  { return _device.mapMemory(memory,offset,size,flags,ppData,*this); }
	inline void unmapMemory(vk::DeviceMemory memory) const  { _device.unmapMemory(memory,*this); }
	inline vk::Result flushMappedMemoryRanges(uint32_t memoryRangeCount,const vk::MappedMemoryRange* pMemoryRanges) const  { return _device.flushMappedMemoryRanges(memoryRangeCount,pMemoryRanges,*this); }

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
	inline vk::Result bindBufferMemory(vk::Buffer buffer,vk::DeviceMemory memory,vk::DeviceSize memoryOffset) const  { return _device.bindBufferMemory(buffer,memory,memoryOffset,*this); }
#endif

#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
	inline PFN_vkVoidFunction getProcAddr(const std::string& name) const  { return (PFN_vkVoidFunction)name.c_str(); }//_device.getProcAddr(name,*this); }
	inline void destroy(vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroy(allocator,*this); }
	inline vk::Queue getQueue(uint32_t queueFamilyIndex,uint32_t queueIndex) const  { return _device.getQueue(queueFamilyIndex,queueIndex,*this); }
	inline vk::ResultValueType<vk::RenderPass>::type createRenderPass(const vk::RenderPassCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createRenderPass(createInfo,allocator,*this); }
	inline void destroyRenderPass(vk::RenderPass renderPass,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroyRenderPass(renderPass,allocator,*this); }
	inline void destroy(vk::RenderPass renderPass,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroyRenderPass(renderPass,allocator,*this); }
	inline vk::ResultValueType<vk::Buffer>::type createBuffer(const vk::BufferCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createBuffer(createInfo,allocator,*this); }
	inline void destroyBuffer(vk::Buffer buffer,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroyBuffer(buffer,allocator,*this); }
	inline void destroy(vk::Buffer buffer,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroyBuffer(buffer,allocator,*this); }
	inline vk::ResultValueType<vk::DeviceMemory>::type allocateMemory(const vk::MemoryAllocateInfo& allocateInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.allocateMemory(allocateInfo,allocator,*this); }
	inline vk::ResultValueType<void>::type bindBufferMemory(vk::Buffer buffer,vk::DeviceMemory memory,vk::DeviceSize memoryOffset) const  { return _device.bindBufferMemory(buffer,memory,memoryOffset,*this); }
	inline void freeMemory(vk::DeviceMemory memory,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.freeMemory(memory,allocator,*this); }
	inline vk::MemoryRequirements getBufferMemoryRequirements(vk::Buffer buffer) const  { return _device.getBufferMemoryRequirements(buffer,*this); }
	inline vk::ResultValueType<void*>::type mapMemory(vk::DeviceMemory memory,vk::DeviceSize offset,vk::DeviceSize size,vk::MemoryMapFlags flags=vk::MemoryMapFlags()) const  { return _device.mapMemory(memory,offset,size,flags,*this); }
	inline vk::ResultValueType<void>::type flushMappedMemoryRanges(vk::ArrayProxy<const vk::MappedMemoryRange> memoryRanges) const  { return _device.flushMappedMemoryRanges(memoryRanges,*this); }

#ifndef VULKAN_HPP_NO_SMART_HANDLE
	inline vk::ResultValueType<vk::UniqueHandle<vk::RenderPass,VulkanDevice>>::type createRenderPassUnique(const vk::RenderPassCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createRenderPassUnique(createInfo,allocator,*this); }
	inline vk::ResultValueType<vk::UniqueHandle<vk::Buffer,VulkanDevice>>::type createBufferUnique(const vk::BufferCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createBufferUnique(createInfo,allocator,*this); }
	inline vk::ResultValueType<vk::UniqueHandle<vk::DeviceMemory,VulkanDevice>>::type allocateMemoryUnique(const vk::MemoryAllocateInfo& allocateInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.allocateMemoryUnique(allocateInfo,allocator,*this); }
#endif
#endif

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
	PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges;

	PFN_vkCreateImageView vkCreateImageView;
	PFN_vkDestroyImageView vkDestroyImageView;
	PFN_vkCreateFramebuffer vkCreateFramebuffer;
	PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
	PFN_vkCreateShaderModule vkCreateShaderModule;
	PFN_vkDestroyShaderModule vkDestroyShaderModule;
	PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
	PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout;
	PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
	PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool;
	PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets;
	PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;
	PFN_vkFreeDescriptorSets vkFreeDescriptorSets;
	PFN_vkCreatePipelineCache vkCreatePipelineCache;
	PFN_vkDestroyPipelineCache vkDestroyPipelineCache;
	PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
	PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
	PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
	PFN_vkCreateComputePipelines vkCreateComputePipelines;
	PFN_vkDestroyPipeline vkDestroyPipeline;
	PFN_vkCreateSemaphore vkCreateSemaphore;
	PFN_vkDestroySemaphore vkDestroySemaphore;
	PFN_vkCreateCommandPool vkCreateCommandPool;
	PFN_vkDestroyCommandPool vkDestroyCommandPool;
	PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
	PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
	PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
	PFN_vkEndCommandBuffer vkEndCommandBuffer;
	PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
	PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
	PFN_vkCmdExecuteCommands vkCmdExecuteCommands;
	PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
	PFN_vkCreateFence vkCreateFence;
	PFN_vkDestroyFence vkDestroyFence;
	PFN_vkCmdBindPipeline vkCmdBindPipeline;
	PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets;
	PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
	PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
	PFN_vkCmdDrawIndexedIndirect vkCmdDrawIndexedIndirect;
	PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
	PFN_vkCmdDraw vkCmdDraw;
	PFN_vkCmdDispatch vkCmdDispatch;
	PFN_vkCmdDispatchIndirect vkCmdDispatchIndirect;
	PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
	PFN_vkQueueSubmit vkQueueSubmit;
	PFN_vkWaitForFences vkWaitForFences;
	PFN_vkQueueWaitIdle vkQueueWaitIdle;
	PFN_vkDeviceWaitIdle vkDeviceWaitIdle;

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
