#include <CadR/VulkanDevice.h>
#include <CadR/VulkanInstance.h>
#include <functional>

using namespace CadR;


void VulkanDevice::init(VulkanInstance& instance,vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo& createInfo)
{
	if(_device)
		cleanUp();

	_device=physicalDevice.createDevice(createInfo,nullptr,instance);
	vkGetDeviceProcAddr  =PFN_vkGetDeviceProcAddr(_device.getProcAddr("vkGetDeviceProcAddr",instance));
	vkDestroyDevice      =getProcAddr<PFN_vkDestroyDevice      >("vkDestroyDevice");
	vkGetDeviceQueue     =getProcAddr<PFN_vkGetDeviceQueue     >("vkGetDeviceQueue");
	vkCreateRenderPass   =getProcAddr<PFN_vkCreateRenderPass   >("vkCreateRenderPass");
	vkDestroyRenderPass  =getProcAddr<PFN_vkDestroyRenderPass  >("vkDestroyRenderPass");
	vkCreateBuffer       =getProcAddr<PFN_vkCreateBuffer       >("vkCreateBuffer");
	vkDestroyBuffer      =getProcAddr<PFN_vkDestroyBuffer      >("vkDestroyBuffer");
	vkAllocateMemory     =getProcAddr<PFN_vkAllocateMemory     >("vkAllocateMemory");
	vkBindBufferMemory   =getProcAddr<PFN_vkBindBufferMemory   >("vkBindBufferMemory");
	vkFreeMemory         =getProcAddr<PFN_vkFreeMemory         >("vkFreeMemory");
	vkGetBufferMemoryRequirements =getProcAddr<PFN_vkGetBufferMemoryRequirements>("vkGetBufferMemoryRequirements");
	vkMapMemory          =getProcAddr<PFN_vkMapMemory          >("vkMapMemory");
	vkUnmapMemory        =getProcAddr<PFN_vkUnmapMemory        >("vkUnmapMemory");
	vkCreateImageView    =getProcAddr<PFN_vkCreateImageView    >("vkCreateImageView");
	vkDestroyImageView   =getProcAddr<PFN_vkDestroyImageView   >("vkDestroyImageView");
	vkCreateFramebuffer  =getProcAddr<PFN_vkCreateFramebuffer  >("vkCreateFramebuffer");
	vkDestroyFramebuffer =getProcAddr<PFN_vkDestroyFramebuffer >("vkDestroyFramebuffer");
	vkCreateCommandPool  =getProcAddr<PFN_vkCreateCommandPool  >("vkCreateCommandPool");
	vkDestroyCommandPool =getProcAddr<PFN_vkDestroyCommandPool >("vkDestroyCommandPool");
	vkAllocateCommandBuffers =getProcAddr<PFN_vkAllocateCommandBuffers>("vkAllocateCommandBuffers");
	vkBeginCommandBuffer =getProcAddr<PFN_vkBeginCommandBuffer >("vkBeginCommandBuffer");
	vkEndCommandBuffer   =getProcAddr<PFN_vkEndCommandBuffer   >("vkEndCommandBuffer");
	vkCmdCopyBuffer      =getProcAddr<PFN_vkCmdCopyBuffer      >("vkCmdCopyBuffer");
	vkCreateFence        =getProcAddr<PFN_vkCreateFence        >("vkCreateFence");
	vkDestroyFence       =getProcAddr<PFN_vkDestroyFence       >("vkDestroyFence");
	vkQueueSubmit        =getProcAddr<PFN_vkQueueSubmit        >("vkQueueSubmit");
	vkWaitForFences      =getProcAddr<PFN_vkWaitForFences      >("vkWaitForFences");

}


void VulkanDevice::init(VulkanInstance& instance,
                        std::tuple<vk::PhysicalDevice,uint32_t,uint32_t> physicalDeviceAndQueueFamilies,
                        const vk::ArrayProxy<const char*const> enabledLayers,
                        const vk::ArrayProxy<const char*const> enabledExtensions,
                        const vk::PhysicalDeviceFeatures* enabledFeatures)
{
	if(_device)
		cleanUp();
	if(!std::get<0>(physicalDeviceAndQueueFamilies))
		return;

	uint32_t graphicsQueueFamily=std::get<1>(physicalDeviceAndQueueFamilies);
	uint32_t presentationQueueFamily=std::get<2>(physicalDeviceAndQueueFamilies);
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
	init(instance,std::get<0>(physicalDeviceAndQueueFamilies),createInfo);
}


void VulkanDevice::cleanUp()
{
	if(_device) {

		// invoke cleanUp handlers
		auto handlers=_cleanUpHandlers;
		for(auto it=handlers.rbegin(); it!=handlers.rend(); it++)
			std::invoke(std::get<0>(*it),reinterpret_cast<VulkanDevice*>(std::get<1>(*it)));

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
		cleanUp();

	*this=other;
	other._device=nullptr;
	other.vkGetDeviceProcAddr=nullptr;
	other.vkDestroyDevice=nullptr;
	return *this;
}
