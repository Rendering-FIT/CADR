#include <CadR/VulkanDevice.h>
#include <CadR/VulkanInstance.h>
#include <functional>

using namespace CadR;


void VulkanDevice::init(VulkanInstance& instance,vk::PhysicalDevice physicalDevice,vk::Device device)
{
	if(_device)
		destroy();

	_device=device;
	_version=instance.getPhysicalDeviceProperties(physicalDevice).apiVersion;

	vkGetDeviceProcAddr  =PFN_vkGetDeviceProcAddr(_device.getProcAddr("vkGetDeviceProcAddr",instance));
	vkDestroyDevice      =getProcAddr<PFN_vkDestroyDevice      >("vkDestroyDevice");
	vkGetDeviceQueue     =getProcAddr<PFN_vkGetDeviceQueue     >("vkGetDeviceQueue");
	vkCreateRenderPass   =getProcAddr<PFN_vkCreateRenderPass   >("vkCreateRenderPass");
	vkDestroyRenderPass  =getProcAddr<PFN_vkDestroyRenderPass  >("vkDestroyRenderPass");
	vkCreateBuffer       =getProcAddr<PFN_vkCreateBuffer       >("vkCreateBuffer");
	vkDestroyBuffer      =getProcAddr<PFN_vkDestroyBuffer      >("vkDestroyBuffer");
	vkGetBufferDeviceAddress=getProcAddr<PFN_vkGetBufferDeviceAddress>("vkGetBufferDeviceAddress");
	vkAllocateMemory     =getProcAddr<PFN_vkAllocateMemory     >("vkAllocateMemory");
	vkBindBufferMemory   =getProcAddr<PFN_vkBindBufferMemory   >("vkBindBufferMemory");
	vkBindImageMemory    =getProcAddr<PFN_vkBindImageMemory    >("vkBindImageMemory");
	vkFreeMemory         =getProcAddr<PFN_vkFreeMemory         >("vkFreeMemory");
	vkGetBufferMemoryRequirements =getProcAddr<PFN_vkGetBufferMemoryRequirements>("vkGetBufferMemoryRequirements");
	vkGetImageMemoryRequirements  =getProcAddr<PFN_vkGetImageMemoryRequirements >("vkGetImageMemoryRequirements");
	vkMapMemory          =getProcAddr<PFN_vkMapMemory          >("vkMapMemory");
	vkUnmapMemory        =getProcAddr<PFN_vkUnmapMemory        >("vkUnmapMemory");
	vkFlushMappedMemoryRanges=getProcAddr<PFN_vkFlushMappedMemoryRanges>("vkFlushMappedMemoryRanges");
	vkCreateImage        =getProcAddr<PFN_vkCreateImage        >("vkCreateImage");
	vkDestroyImage       =getProcAddr<PFN_vkDestroyImage       >("vkDestroyImage");
	vkCreateImageView    =getProcAddr<PFN_vkCreateImageView    >("vkCreateImageView");
	vkDestroyImageView   =getProcAddr<PFN_vkDestroyImageView   >("vkDestroyImageView");
	vkCreateFramebuffer  =getProcAddr<PFN_vkCreateFramebuffer  >("vkCreateFramebuffer");
	vkDestroyFramebuffer =getProcAddr<PFN_vkDestroyFramebuffer >("vkDestroyFramebuffer");
	vkCreateShaderModule =getProcAddr<PFN_vkCreateShaderModule >("vkCreateShaderModule");
	vkDestroyShaderModule=getProcAddr<PFN_vkDestroyShaderModule>("vkDestroyShaderModule");
	vkCreateDescriptorSetLayout=getProcAddr<PFN_vkCreateDescriptorSetLayout>("vkCreateDescriptorSetLayout");
	vkDestroyDescriptorSetLayout=getProcAddr<PFN_vkDestroyDescriptorSetLayout>("vkDestroyDescriptorSetLayout");
	vkCreateDescriptorPool=getProcAddr<PFN_vkCreateDescriptorPool>("vkCreateDescriptorPool");
	vkDestroyDescriptorPool=getProcAddr<PFN_vkDestroyDescriptorPool>("vkDestroyDescriptorPool");
	vkResetDescriptorPool=getProcAddr<PFN_vkResetDescriptorPool>("vkResetDescriptorPool");
	vkAllocateDescriptorSets=getProcAddr<PFN_vkAllocateDescriptorSets>("vkAllocateDescriptorSets");
	vkUpdateDescriptorSets=getProcAddr<PFN_vkUpdateDescriptorSets>("vkUpdateDescriptorSets");
	vkFreeDescriptorSets =getProcAddr<PFN_vkFreeDescriptorSets >("vkFreeDescriptorSets");
	vkCreatePipelineCache=getProcAddr<PFN_vkCreatePipelineCache>("vkCreatePipelineCache");
	vkDestroyPipelineCache=getProcAddr<PFN_vkDestroyPipelineCache>("vkDestroyPipelineCache");
	vkCreatePipelineLayout=getProcAddr<PFN_vkCreatePipelineLayout>("vkCreatePipelineLayout");
	vkDestroyPipelineLayout=getProcAddr<PFN_vkDestroyPipelineLayout>("vkDestroyPipelineLayout");
	vkCreateGraphicsPipelines=getProcAddr<PFN_vkCreateGraphicsPipelines>("vkCreateGraphicsPipelines");
	vkCreateComputePipelines=getProcAddr<PFN_vkCreateComputePipelines>("vkCreateComputePipelines");
	vkDestroyPipeline    =getProcAddr<PFN_vkDestroyPipeline    >("vkDestroyPipeline");
	vkCreateSemaphore    =getProcAddr<PFN_vkCreateSemaphore    >("vkCreateSemaphore");
	vkDestroySemaphore   =getProcAddr<PFN_vkDestroySemaphore   >("vkDestroySemaphore");
	vkCreateCommandPool  =getProcAddr<PFN_vkCreateCommandPool  >("vkCreateCommandPool");
	vkDestroyCommandPool =getProcAddr<PFN_vkDestroyCommandPool >("vkDestroyCommandPool");
	vkAllocateCommandBuffers=getProcAddr<PFN_vkAllocateCommandBuffers>("vkAllocateCommandBuffers");
	vkFreeCommandBuffers =getProcAddr<PFN_vkFreeCommandBuffers >("vkFreeCommandBuffers");
	vkBeginCommandBuffer =getProcAddr<PFN_vkBeginCommandBuffer >("vkBeginCommandBuffer");
	vkEndCommandBuffer   =getProcAddr<PFN_vkEndCommandBuffer   >("vkEndCommandBuffer");
	vkResetCommandPool   =getProcAddr<PFN_vkResetCommandPool   >("vkResetCommandPool");
	vkCmdPushConstants   =getProcAddr<PFN_vkCmdPushConstants   >("vkCmdPushConstants");
	vkCmdBeginRenderPass =getProcAddr<PFN_vkCmdBeginRenderPass >("vkCmdBeginRenderPass");
	vkCmdEndRenderPass   =getProcAddr<PFN_vkCmdEndRenderPass   >("vkCmdEndRenderPass");
	vkCmdExecuteCommands =getProcAddr<PFN_vkCmdExecuteCommands >("vkCmdExecuteCommands");
	vkCmdCopyBuffer      =getProcAddr<PFN_vkCmdCopyBuffer      >("vkCmdCopyBuffer");
	vkCreateFence        =getProcAddr<PFN_vkCreateFence        >("vkCreateFence");
	vkDestroyFence       =getProcAddr<PFN_vkDestroyFence       >("vkDestroyFence");
	vkCmdBindPipeline    =getProcAddr<PFN_vkCmdBindPipeline    >("vkCmdBindPipeline");
	vkCmdBindDescriptorSets=getProcAddr<PFN_vkCmdBindDescriptorSets>("vkCmdBindDescriptorSets");
	vkCmdBindIndexBuffer =getProcAddr<PFN_vkCmdBindIndexBuffer >("vkCmdBindIndexBuffer");
	vkCmdBindVertexBuffers=getProcAddr<PFN_vkCmdBindVertexBuffers>("vkCmdBindVertexBuffers");
	vkCmdDrawIndexedIndirect=getProcAddr<PFN_vkCmdDrawIndexedIndirect>("vkCmdDrawIndexedIndirect");
	vkCmdDrawIndexed     =getProcAddr<PFN_vkCmdDrawIndexed     >("vkCmdDrawIndexed");
	vkCmdDraw            =getProcAddr<PFN_vkCmdDraw            >("vkCmdDraw");
	vkCmdDispatch        =getProcAddr<PFN_vkCmdDispatch        >("vkCmdDispatch");
	vkCmdDispatchIndirect=getProcAddr<PFN_vkCmdDispatchIndirect>("vkCmdDispatchIndirect");
	vkCmdPipelineBarrier =getProcAddr<PFN_vkCmdPipelineBarrier >("vkCmdPipelineBarrier");
	vkCmdSetDepthBias    =getProcAddr<PFN_vkCmdSetDepthBias    >("vkCmdSetDepthBias");
	vkCmdSetLineWidth    =getProcAddr<PFN_vkCmdSetLineWidth    >("vkCmdSetLineWidth");
	vkCmdSetLineStippleEXT=getProcAddr<PFN_vkCmdSetLineStippleEXT>("vkCmdSetLineStippleEXT");
	vkQueueSubmit        =getProcAddr<PFN_vkQueueSubmit        >("vkQueueSubmit");
	vkWaitForFences      =getProcAddr<PFN_vkWaitForFences      >("vkWaitForFences");
	vkResetFences        =getProcAddr<PFN_vkResetFences        >("vkResetFences");
	vkQueueWaitIdle      =getProcAddr<PFN_vkQueueWaitIdle      >("vkQueueWaitIdle");
	vkDeviceWaitIdle     =getProcAddr<PFN_vkDeviceWaitIdle     >("vkDeviceWaitIdle");
	vkCmdResetQueryPool  =getProcAddr<PFN_vkCmdResetQueryPool  >("vkCmdResetQueryPool");
	vkCmdWriteTimestamp  =getProcAddr<PFN_vkCmdWriteTimestamp  >("vkCmdWriteTimestamp");
	vkGetCalibratedTimestampsEXT=getProcAddr<PFN_vkGetCalibratedTimestampsEXT>("vkGetCalibratedTimestampsEXT");
	vkCreateQueryPool    =getProcAddr<PFN_vkCreateQueryPool    >("vkCreateQueryPool");
	vkDestroyQueryPool   =getProcAddr<PFN_vkDestroyQueryPool   >("vkDestroyQueryPool");
	vkGetQueryPoolResults=getProcAddr<PFN_vkGetQueryPoolResults>("vkGetQueryPoolResults");
}


void VulkanDevice::init(VulkanInstance& instance,vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo& createInfo)
{
	init(instance,physicalDevice,instance.createDevice(physicalDevice,createInfo));
}


void VulkanDevice::init(VulkanInstance& instance,
                        vk::PhysicalDevice physicalDevice,
                        uint32_t graphicsQueueFamily,
                        uint32_t presentationQueueFamily,
                        const vk::ArrayProxy<const char*const> enabledLayers,
                        const vk::ArrayProxy<const char*const> enabledExtensions,
                        const vk::PhysicalDeviceFeatures* enabledFeatures)
{
	size_t numQueues=(graphicsQueueFamily==presentationQueueFamily)?1:2;
	std::array<vk::DeviceQueueCreateInfo,2> queueInfos = {
		vk::DeviceQueueCreateInfo{
			vk::DeviceQueueCreateFlags(),  // flags
			graphicsQueueFamily,  // queueFamilyIndex
			1,  // queueCount
			&(const float&)1.f,  // queuePriorities
		},
		{
			vk::DeviceQueueCreateFlags(),  // flags
			presentationQueueFamily,  // queueFamilyIndex
			1,  // queueCount
			&(const float&)1.f,  // queuePriorities
		},
	};
	vk::DeviceCreateInfo createInfo(
		vk::DeviceCreateFlags(),   // flags
		uint32_t(numQueues),queueInfos.data(),  // queueCreateInfo
		enabledLayers.size(),enabledLayers.data(),  // enabledLayersCount
		enabledExtensions.size(),enabledExtensions.data(),  // enabledExtensions
		enabledFeatures  // enabledFeatures
	);
	init(instance,physicalDevice,createInfo);
}


void VulkanDevice::init(VulkanInstance& instance,
                          vk::PhysicalDevice physicalDevice,
                          uint32_t graphicsQueueFamily,
                          uint32_t presentationQueueFamily,
                          const vk::ArrayProxy<const char*const> enabledLayers,
                          const vk::ArrayProxy<const char*const> enabledExtensions,
                          const vk::PhysicalDeviceFeatures2* enabledFeatures)
{
	size_t numQueues=(graphicsQueueFamily==presentationQueueFamily)?1:2;
	std::array<vk::DeviceQueueCreateInfo,2> queueInfos = {
		vk::DeviceQueueCreateInfo{
			vk::DeviceQueueCreateFlags(),  // flags
			graphicsQueueFamily,  // queueFamilyIndex
			1,  // queueCount
			&(const float&)1.f,  // queuePriorities
		},
		{
			vk::DeviceQueueCreateFlags(),  // flags
			presentationQueueFamily,  // queueFamilyIndex
			1,  // queueCount
			&(const float&)1.f,  // queuePriorities
		},
	};
	vk::DeviceCreateInfo createInfo(
		vk::DeviceCreateFlags(),   // flags
		uint32_t(numQueues),queueInfos.data(),  // queueCreateInfo
		enabledLayers.size(),enabledLayers.data(),  // enabledLayersCount
		enabledExtensions.size(),enabledExtensions.data(),  // enabledExtensions
		nullptr  // enabledFeatures
	);
	createInfo.setPNext(enabledFeatures);
	init(instance,physicalDevice,createInfo);
}


void VulkanDevice::create(VulkanInstance& instance,vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo& createInfo)
{
	if(_device)
		destroy();
	if(!physicalDevice)
		return;

	init(instance,physicalDevice,createInfo);
}


void VulkanDevice::create(VulkanInstance& instance,
                          vk::PhysicalDevice physicalDevice,
                          uint32_t graphicsQueueFamily,
                          uint32_t presentationQueueFamily,
                          const vk::ArrayProxy<const char*const> enabledLayers,
                          const vk::ArrayProxy<const char*const> enabledExtensions,
                          const vk::PhysicalDeviceFeatures* enabledFeatures)
{
	if(_device)
		destroy();
	if(!physicalDevice)
		return;

	init(instance,physicalDevice,graphicsQueueFamily,presentationQueueFamily,enabledLayers,enabledExtensions,enabledFeatures);
}


void VulkanDevice::create(VulkanInstance& instance,
                          vk::PhysicalDevice physicalDevice,
                          uint32_t graphicsQueueFamily,
                          uint32_t presentationQueueFamily,
                          const vk::ArrayProxy<const char*const> enabledLayers,
                          const vk::ArrayProxy<const char*const> enabledExtensions,
                          const vk::PhysicalDeviceFeatures2* enabledFeatures)
{
	if(_device)
		destroy();
	if(!physicalDevice)
		return;

	init(instance,physicalDevice,graphicsQueueFamily,presentationQueueFamily,enabledLayers,enabledExtensions,enabledFeatures);
}


void VulkanDevice::destroy()
{
	if(_device) {

		// invoke cleanUp handlers
		cleanUpCallbacks.invoke();

		// destroy device
		_device.destroy(nullptr,*this);
		_device=nullptr;
		vkGetDeviceProcAddr=nullptr;
		vkDestroyDevice=nullptr;

	}
}


VulkanDevice::VulkanDevice(VulkanDevice&& other) noexcept
	: VulkanDevice(other)
{
	other._device=nullptr;
	other.vkGetDeviceProcAddr=nullptr;
	other.vkDestroyDevice=nullptr;
}


VulkanDevice& VulkanDevice::operator=(VulkanDevice&& other) noexcept
{
	if(_device)
		destroy();

	*this=other;
	other._device=nullptr;
	other.vkGetDeviceProcAddr=nullptr;
	other.vkDestroyDevice=nullptr;
	return *this;
}
