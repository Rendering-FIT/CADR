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
	inline vk::Result createImageView(const vk::ImageViewCreateInfo* pCreateInfo,const vk::AllocationCallbacks* pAllocator,vk::ImageView* pView) const  { return _device.createImageView(pCreateInfo,pAllocator,pView,*this); }
	inline void destroyImageView(vk::ImageView imageView,const vk::AllocationCallbacks* pAllocator) const  { _device.destroyImageView(imageView,pAllocator,*this); }
	inline void destroy(vk::ImageView imageView,const vk::AllocationCallbacks* pAllocator) const  { _device.destroy(imageView,pAllocator,*this); }
	inline vk::Result createFramebuffer(const vk::FramebufferCreateInfo* pCreateInfo,const vk::AllocationCallbacks* pAllocator,vk::Framebuffer* pFramebuffer) const  { return _device.createFramebuffer(pCreateInfo,pAllocator,pFramebuffer,*this); }
	inline void destroyFramebuffer(vk::Framebuffer framebuffer,const vk::AllocationCallbacks* pAllocator) const  { _device.destroyFramebuffer(framebuffer,pAllocator,*this); }
	inline void destroy(vk::Framebuffer framebuffer,const vk::AllocationCallbacks* pAllocator) const  { _device.destroy(framebuffer,pAllocator,*this); }
	inline vk::Result createShaderModule(const vk::ShaderModuleCreateInfo* pCreateInfo,const vk::AllocationCallbacks* pAllocator,vk::ShaderModule* pShaderModule) const  { return _device.createShaderModule(pCreateInfo,pAllocator,pShaderModule,*this); }
	inline void destroyShaderModule(vk::ShaderModule shaderModule,const vk::AllocationCallbacks* pAllocator) const  { _device.destroyShaderModule(shaderModule,pAllocator,*this); }
	inline void destroy(vk::ShaderModule shaderModule,const vk::AllocationCallbacks* pAllocator) const  { _device.destroy(shaderModule,pAllocator,*this); }
	inline vk::Result createDescriptorSetLayout(const vk::DescriptorSetLayoutCreateInfo* pCreateInfo,const vk::AllocationCallbacks* pAllocator,vk::DescriptorSetLayout* pSetLayout) const  { return _device.createDescriptorSetLayout(pCreateInfo,pAllocator,pSetLayout,*this); }
	inline void destroyDescriptorSetLayout(vk::DescriptorSetLayout descriptorSetLayout,const vk::AllocationCallbacks* pAllocator) const  { _device.destroyDescriptorSetLayout(descriptorSetLayout,pAllocator,*this); }
	inline void destroy(vk::DescriptorSetLayout descriptorSetLayout,const vk::AllocationCallbacks* pAllocator) const  { _device.destroy(descriptorSetLayout,pAllocator,*this); }
	inline vk::Result createDescriptorPool(const vk::DescriptorPoolCreateInfo* pCreateInfo,const vk::AllocationCallbacks* pAllocator,vk::DescriptorPool* pDescriptorPool) const  { return _device.createDescriptorPool(pCreateInfo,pAllocator,pDescriptorPool,*this); }
	inline void destroyDescriptorPool(vk::DescriptorPool descriptorPool,const vk::AllocationCallbacks* pAllocator) const  { _device.destroyDescriptorPool(descriptorPool,pAllocator,*this); }
	inline void destroy(vk::DescriptorPool descriptorPool,const vk::AllocationCallbacks* pAllocator) const  { _device.destroy(descriptorPool,pAllocator,*this); }
	inline vk::Result allocateDescriptorSets(const vk::DescriptorSetAllocateInfo* pAllocateInfo,vk::DescriptorSet* pDescriptorSets) const  { return _device.allocateDescriptorSets(pAllocateInfo,pDescriptorSets,*this); }
	inline void updateDescriptorSets(uint32_t descriptorWriteCount,const vk::WriteDescriptorSet* pDescriptorWrites,uint32_t descriptorCopyCount,const vk::CopyDescriptorSet* pDescriptorCopies) const  { _device.updateDescriptorSets(descriptorWriteCount,pDescriptorWrites,descriptorCopyCount,pDescriptorCopies,*this); }
	inline vk::Result freeDescriptorSets(vk::DescriptorPool descriptorPool,uint32_t descriptorSetCount,const vk::DescriptorSet* pDescriptorSets) const  { return _device.freeDescriptorSets(descriptorPool,descriptorSetCount,pDescriptorSets,*this); }
	inline vk::Result free(vk::DescriptorPool descriptorPool,uint32_t descriptorSetCount,const vk::DescriptorSet* pDescriptorSets) const  { return _device.free(descriptorPool,descriptorSetCount,pDescriptorSets,*this); }
	inline vk::Result createPipelineCache(const vk::PipelineCacheCreateInfo* pCreateInfo,const vk::AllocationCallbacks* pAllocator,vk::PipelineCache* pPipelineCache) const  { return _device.createPipelineCache(pCreateInfo,pAllocator,pPipelineCache,*this); }
	inline void destroyPipelineCache(vk::PipelineCache pipelineCache,const vk::AllocationCallbacks* pAllocator) const  { _device.destroyPipelineCache(pipelineCache,pAllocator,*this); }
	inline void destroy(vk::PipelineCache pipelineCache,const vk::AllocationCallbacks* pAllocator) const  { _device.destroy(pipelineCache,pAllocator,*this); }
	inline vk::Result createPipelineLayout(const vk::PipelineLayoutCreateInfo* pCreateInfo,const vk::AllocationCallbacks* pAllocator,vk::PipelineLayout* pPipelineLayout) const  { return _device.createPipelineLayout(pCreateInfo,pAllocator,pPipelineLayout,*this); }
	inline void destroyPipelineLayout(vk::PipelineLayout pipelineLayout,const vk::AllocationCallbacks* pAllocator) const  { _device.destroyPipelineLayout(pipelineLayout,pAllocator,*this); }
	inline void destroy(vk::PipelineLayout pipelineLayout,const vk::AllocationCallbacks* pAllocator) const  { _device.destroy(pipelineLayout,pAllocator,*this); }
	inline vk::Result createGraphicsPipelines(vk::PipelineCache pipelineCache,uint32_t createInfoCount,const vk::GraphicsPipelineCreateInfo* pCreateInfos,const vk::AllocationCallbacks* pAllocator,vk::Pipeline* pPipelines) const  { return _device.createGraphicsPipelines(pipelineCache,createInfoCount,pCreateInfos,pAllocator,pPipelines,*this); }
	inline vk::Result createComputePipelines(vk::PipelineCache pipelineCache,uint32_t createInfoCount,const vk::ComputePipelineCreateInfo* pCreateInfos,const vk::AllocationCallbacks* pAllocator,vk::Pipeline* pPipelines) const  { return _device.createComputePipelines(pipelineCache,createInfoCount,pCreateInfos,pAllocator,pPipelines,*this); }
	inline void destroyPipeline(vk::Pipeline pipeline,const vk::AllocationCallbacks* pAllocator) const  { _device.destroyPipeline(pipeline,pAllocator,*this); }
	inline void destroy(vk::Pipeline pipeline,const vk::AllocationCallbacks* pAllocator) const  { _device.destroy(pipeline,pAllocator,*this); }

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
	inline vk::ResultValueType<vk::ImageView>::type createImageView(const vk::ImageViewCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createImageView(createInfo,allocator,*this); }
	inline void destroyImageView(vk::ImageView imageView,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroyImageView(imageView,allocator,*this); }
	inline void destroy(vk::ImageView imageView,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroy(imageView,allocator,*this); }
	inline vk::ResultValueType<vk::Framebuffer>::type createFramebuffer(const vk::FramebufferCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createFramebuffer(createInfo,allocator,*this); }
	inline void destroyFramebuffer(vk::Framebuffer framebuffer,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroyFramebuffer(framebuffer,allocator,*this); }
	inline void destroy(vk::Framebuffer framebuffer,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroy(framebuffer,allocator,*this); }
	inline vk::ResultValueType<vk::ShaderModule>::type createShaderModule(const vk::ShaderModuleCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createShaderModule(createInfo,allocator,*this); }
	inline void destroyShaderModule(vk::ShaderModule shaderModule,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroyShaderModule(shaderModule,allocator,*this); }
	inline void destroy(vk::ShaderModule shaderModule,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroy(shaderModule,allocator,*this); }
	inline vk::ResultValueType<vk::DescriptorSetLayout>::type createDescriptorSetLayout(const vk::DescriptorSetLayoutCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createDescriptorSetLayout(createInfo,allocator,*this); }
	inline void destroyDescriptorSetLayout(vk::DescriptorSetLayout descriptorSetLayout,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroyDescriptorSetLayout(descriptorSetLayout,allocator,*this); }
	inline void destroy(vk::DescriptorSetLayout descriptorSetLayout,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroyDescriptorSetLayout(descriptorSetLayout,allocator,*this); }
	inline vk::ResultValueType<vk::DescriptorPool>::type createDescriptorPool(const vk::DescriptorPoolCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createDescriptorPool(createInfo,allocator,*this); }
	inline void destroyDescriptorPool(vk::DescriptorPool descriptorPool,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroyDescriptorPool(descriptorPool,allocator,*this); }
	inline void destroy(vk::DescriptorPool descriptorPool,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroy(descriptorPool,allocator,*this); }
	template<typename Allocator=std::allocator<vk::DescriptorSet>>
	typename vk::ResultValueType<std::vector<vk::DescriptorSet,Allocator>>::type allocateDescriptorSets(const vk::DescriptorSetAllocateInfo& allocateInfo) const  { return _device.allocateDescriptorSets(allocateInfo,*this); }
	template<typename Allocator=std::allocator<vk::DescriptorSet>>
	typename vk::ResultValueType<std::vector<vk::DescriptorSet,Allocator>>::type allocateDescriptorSets(const vk::DescriptorSetAllocateInfo& allocateInfo,Allocator const& vectorAllocator) const  { return _device.allocateDescriptorSets(allocateInfo,vectorAllocator,*this); }
	inline void updateDescriptorSets(vk::ArrayProxy<const vk::WriteDescriptorSet> descriptorWrites,vk::ArrayProxy<const vk::CopyDescriptorSet> descriptorCopies) const  { _device.updateDescriptorSets(descriptorWrites,descriptorCopies,*this); }
	inline vk::ResultValueType<void>::type freeDescriptorSets(vk::DescriptorPool descriptorPool,vk::ArrayProxy<const vk::DescriptorSet> descriptorSets) const  { return _device.freeDescriptorSets(descriptorPool,descriptorSets,*this); }
	inline vk::ResultValueType<void>::type free(vk::DescriptorPool descriptorPool,vk::ArrayProxy<const vk::DescriptorSet> descriptorSets) const  { return _device.free(descriptorPool,descriptorSets,*this); }
	inline vk::ResultValueType<vk::PipelineCache>::type createPipelineCache(const vk::PipelineCacheCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createPipelineCache(createInfo,allocator,*this); }
	inline void destroyPipelineCache(vk::PipelineCache pipelineCache,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroyPipelineCache(pipelineCache,allocator,*this); }
	inline void destroy(vk::PipelineCache pipelineCache,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroy(pipelineCache,allocator,*this); }
	inline vk::ResultValueType<vk::PipelineLayout>::type createPipelineLayout(const vk::PipelineLayoutCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createPipelineLayout(createInfo,allocator,*this); }
	inline void destroyPipelineLayout(vk::PipelineLayout pipelineLayout,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroyPipelineLayout(pipelineLayout,allocator,*this); }
	inline void destroy(vk::PipelineLayout pipelineLayout,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroy(pipelineLayout,allocator,*this); }
	template<typename Allocator = std::allocator<vk::Pipeline>>
	typename vk::ResultValueType<std::vector<vk::Pipeline,Allocator>>::type createGraphicsPipelines(vk::PipelineCache pipelineCache,vk::ArrayProxy<const vk::GraphicsPipelineCreateInfo> createInfos,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createGraphicsPipelines(pipelineCache,createInfos,allocator,*this); }
	template<typename Allocator = std::allocator<vk::Pipeline>>
	typename vk::ResultValueType<std::vector<vk::Pipeline,Allocator>>::type createGraphicsPipelines(vk::PipelineCache pipelineCache,vk::ArrayProxy<const vk::GraphicsPipelineCreateInfo> createInfos,vk::Optional<const vk::AllocationCallbacks> allocator,Allocator const& vectorAllocator) const  { return _device.createGraphicsPipelines(pipelineCache,createInfos,allocator,vectorAllocator,*this); }
	inline vk::ResultValueType<vk::Pipeline>::type createGraphicsPipeline(vk::PipelineCache pipelineCache,const vk::GraphicsPipelineCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createGraphicsPipeline(pipelineCache,createInfo,allocator,*this); }
	template<typename Allocator=std::allocator<vk::Pipeline>>
	typename vk::ResultValueType<std::vector<vk::Pipeline,Allocator>>::type createComputePipelines(vk::PipelineCache pipelineCache,vk::ArrayProxy<const vk::ComputePipelineCreateInfo> createInfos,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createComputePipelines(pipelineCache,createInfos,allocator,*this); }
	template<typename Allocator = std::allocator<vk::Pipeline>>
	typename vk::ResultValueType<std::vector<vk::Pipeline,Allocator>>::type createComputePipelines(vk::PipelineCache pipelineCache,vk::ArrayProxy<const vk::ComputePipelineCreateInfo> createInfos,vk::Optional<const vk::AllocationCallbacks> allocator,Allocator const& vectorAllocator) const  { return _device.createComputePipelines(pipelineCache,createInfos,allocator,vectorAllocator,*this); }
	inline vk::ResultValueType<vk::Pipeline>::type createComputePipeline(vk::PipelineCache pipelineCache,const vk::ComputePipelineCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createComputePipeline(pipelineCache,createInfo,allocator,*this); }
	inline void destroyPipeline(vk::Pipeline pipeline,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroyPipeline(pipeline,allocator,*this); }
	inline void destroy(vk::Pipeline pipeline,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { _device.destroy(pipeline,allocator,*this); }

#ifndef VULKAN_HPP_NO_SMART_HANDLE
	inline vk::ResultValueType<vk::UniqueHandle<vk::RenderPass,VulkanDevice>>::type createRenderPassUnique(const vk::RenderPassCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createRenderPassUnique(createInfo,allocator,*this); }
	inline vk::ResultValueType<vk::UniqueHandle<vk::Buffer,VulkanDevice>>::type createBufferUnique(const vk::BufferCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createBufferUnique(createInfo,allocator,*this); }
	inline vk::ResultValueType<vk::UniqueHandle<vk::DeviceMemory,VulkanDevice>>::type allocateMemoryUnique(const vk::MemoryAllocateInfo& allocateInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.allocateMemoryUnique(allocateInfo,allocator,*this); }
	inline vk::ResultValueType<vk::UniqueHandle<vk::ImageView,VulkanDevice>>::type createImageViewUnique(const vk::ImageViewCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createImageViewUnique(createInfo,allocator,*this); }
	inline vk::ResultValueType<vk::UniqueHandle<vk::Framebuffer,VulkanDevice>>::type createFramebufferUnique(const vk::FramebufferCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createFramebufferUnique(createInfo,allocator,*this); }
	inline vk::ResultValueType<vk::UniqueHandle<vk::ShaderModule,VulkanDevice>>::type createShaderModuleUnique(const vk::ShaderModuleCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createShaderModuleUnique(createInfo,allocator,*this); }
	inline vk::ResultValueType<vk::UniqueHandle<vk::DescriptorSetLayout,VulkanDevice>>::type createDescriptorSetLayoutUnique(const vk::DescriptorSetLayoutCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createDescriptorSetLayoutUnique(createInfo,allocator,*this); }
	inline vk::ResultValueType<vk::UniqueHandle<vk::DescriptorPool,VulkanDevice>>::type createDescriptorPoolUnique(const vk::DescriptorPoolCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createDescriptorPoolUnique(createInfo,allocator,*this); }
	template<typename Allocator=std::allocator<vk::UniqueDescriptorSet>>
	typename vk::ResultValueType<std::vector<vk::UniqueHandle<vk::DescriptorSet,VulkanDevice>,Allocator>>::type allocateDescriptorSetsUnique(const vk::DescriptorSetAllocateInfo& allocateInfo) const  { return _device.allocateDescriptorSetsUnique(allocateInfo,*this); }
	template<typename Allocator=std::allocator<vk::UniqueDescriptorSet>>
	typename vk::ResultValueType<std::vector<vk::UniqueHandle<vk::DescriptorSet,VulkanDevice>,Allocator>>::type allocateDescriptorSetsUnique(const vk::DescriptorSetAllocateInfo& allocateInfo,Allocator const& vectorAllocator) const  { return _device.allocateDescriptorSetsUnique(allocateInfo,vectorAllocator,*this); }
	inline vk::ResultValueType<vk::UniqueHandle<vk::PipelineCache,VulkanDevice>>::type createPipelineCacheUnique(const vk::PipelineCacheCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createPipelineCacheUnique(createInfo,allocator,*this); }
	inline vk::ResultValueType<vk::UniqueHandle<vk::PipelineLayout,VulkanDevice>>::type createPipelineLayoutUnique(const vk::PipelineLayoutCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createPipelineLayoutUnique(createInfo,allocator,*this); }
	template<typename Allocator=std::allocator<vk::UniquePipeline>>
	typename vk::ResultValueType<std::vector<vk::UniqueHandle<vk::Pipeline,VulkanDevice>,Allocator>>::type createGraphicsPipelinesUnique(vk::PipelineCache pipelineCache,vk::ArrayProxy<const vk::GraphicsPipelineCreateInfo> createInfos,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createGraphicsPipelinesUnique(pipelineCache,createInfos,allocator,*this); }
	template<typename Allocator=std::allocator<vk::UniquePipeline>>
	typename vk::ResultValueType<std::vector<vk::UniqueHandle<vk::Pipeline,VulkanDevice>,Allocator>>::type createGraphicsPipelinesUnique(vk::PipelineCache pipelineCache,vk::ArrayProxy<const vk::GraphicsPipelineCreateInfo> createInfos,vk::Optional<const vk::AllocationCallbacks> allocator,Allocator const& vectorAllocator) const  { return _device.createGraphicsPipelinesUnique(pipelineCache,createInfos,allocator,vectorAllocator,*this); }
	inline vk::ResultValueType<vk::UniqueHandle<vk::Pipeline,VulkanDevice>>::type createGraphicsPipelineUnique(vk::PipelineCache pipelineCache,const vk::GraphicsPipelineCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createGraphicsPipelineUnique(pipelineCache,createInfo,allocator,*this); }
	template<typename Allocator=std::allocator<vk::UniquePipeline>>
	typename vk::ResultValueType<std::vector<vk::UniqueHandle<vk::Pipeline,VulkanDevice>,Allocator>>::type createComputePipelinesUnique(vk::PipelineCache pipelineCache,vk::ArrayProxy<const vk::ComputePipelineCreateInfo> createInfos,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createComputePipelinesUnique(pipelineCache,createInfos,allocator,*this); }
	template<typename Allocator=std::allocator<vk::UniquePipeline>>
	typename vk::ResultValueType<std::vector<vk::UniqueHandle<vk::Pipeline,VulkanDevice>,Allocator>>::type createComputePipelinesUnique(vk::PipelineCache pipelineCache,vk::ArrayProxy<const vk::ComputePipelineCreateInfo> createInfos,vk::Optional<const vk::AllocationCallbacks> allocator,Allocator const& vectorAllocator) const  { return _device.createComputePipelinesUnique(pipelineCache,createInfos,allocator,vectorAllocator,*this); }
	inline vk::ResultValueType<vk::UniqueHandle<vk::Pipeline,VulkanDevice>>::type createComputePipelineUnique(vk::PipelineCache pipelineCache,const vk::ComputePipelineCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return _device.createComputePipelineUnique(pipelineCache,createInfo,allocator,*this); }
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
