#include <CadR/VulkanDevice.h>
#include <CadR/VulkanInstance.h>

using namespace CadR;


void VulkanDevice::init(VulkanInstance& instance,vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo& createInfo)
{
	if(_device)
		reset();

	_device=physicalDevice.createDevice(createInfo,nullptr,instance);
	vkGetDeviceProcAddr=PFN_vkGetDeviceProcAddr(_device.getProcAddr("vkGetDeviceProcAddr",instance));
	vkDestroyDevice=PFN_vkDestroyDevice(_device.getProcAddr("vkDestroyDevice",*this));
}


void VulkanDevice::reset()
{
	if(_device) {
		_device.destroy(nullptr,*this);
		_device=nullptr;
		vkGetDeviceProcAddr=nullptr;
		vkDestroyDevice=nullptr;
	}
}


VulkanDevice::VulkanDevice(VulkanDevice&& other)
	: VulkanDevice(other)
{
	other._device=nullptr;
	other.vkGetDeviceProcAddr=nullptr;
	other.vkDestroyDevice=nullptr;
}


VulkanDevice& VulkanDevice::operator=(VulkanDevice&& other)
{
	if(_device)
		reset();

	*this=other;
	other._device=nullptr;
	other.vkGetDeviceProcAddr=nullptr;
	other.vkDestroyDevice=nullptr;
	return *this;
}
