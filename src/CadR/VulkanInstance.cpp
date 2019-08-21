#include <CadR/VulkanInstance.h>
#include <CadR/VulkanLibrary.h>

using namespace CadR;


void VulkanInstance::init(VulkanLibrary& lib,const vk::InstanceCreateInfo& createInfo)
{
	// avoid multiple initialization attempts
	if(_instance)
		throw std::runtime_error("Multiple initialization attempts.");

	// make sure lib is initialized
	if(!lib.initialized() || lib.vkGetInstanceProcAddr==nullptr)
		throw std::runtime_error("VulkanLibrary class was not initialized.");

	// create instance
	// (Handle the special case of non-1.0 Vulkan version request on Vulkan 1.0 system that throws. See vkCreateInstance() documentation.)
	if(createInfo.pApplicationInfo && createInfo.pApplicationInfo->apiVersion!=VK_API_VERSION_1_0)
	{
		if(lib.enumerateInstanceVersion()==VK_API_VERSION_1_0)
		{
			// replace requested Vulkan version by 1.0 to avoid throwing the exception
			vk::ApplicationInfo appInfo(*createInfo.pApplicationInfo);
			appInfo.apiVersion=VK_API_VERSION_1_0;
			vk::InstanceCreateInfo createInfo2(createInfo);
			createInfo2.pApplicationInfo=&appInfo;
			_instance=vk::createInstance(createInfo2,nullptr,lib);
		}
		else
			_instance=vk::createInstance(createInfo,nullptr,lib);
	}
	else
		_instance=vk::createInstance(createInfo,nullptr,lib);

	// get function pointers
	vkDestroyInstance=getProcAddr<PFN_vkDestroyInstance>(lib,"vkDestroyInstance");
	vkGetPhysicalDeviceProperties=getProcAddr<PFN_vkGetPhysicalDeviceProperties>(lib,"vkGetPhysicalDeviceProperties");
	vkEnumeratePhysicalDevices=getProcAddr<PFN_vkEnumeratePhysicalDevices>(lib,"vkEnumeratePhysicalDevices");
	vkCreateDevice=getProcAddr<PFN_vkCreateDevice>(lib,"vkCreateDevice");
	vkGetDeviceProcAddr=getProcAddr<PFN_vkGetDeviceProcAddr>(lib,"vkGetDeviceProcAddr");
}


void VulkanInstance::reset()
{
	if(_instance) {
		_instance.destroy(nullptr,*this);
		_instance=nullptr;
		vkDestroyInstance=nullptr;
		vkCreateDevice=nullptr;
		vkGetDeviceProcAddr=nullptr;
	}
}


VulkanInstance::VulkanInstance(VulkanInstance&& other)
	: VulkanInstance(other)
{
	other._instance=nullptr;
	other.vkDestroyInstance=nullptr;
	other.vkCreateDevice=nullptr;
	other.vkGetDeviceProcAddr=nullptr;
}


VulkanInstance& VulkanInstance::operator=(VulkanInstance&& other)
{
	if(_instance)
		reset();

	*this=other;
	other._instance=nullptr;
	other.vkDestroyInstance=nullptr;
	other.vkCreateDevice=nullptr;
	other.vkGetDeviceProcAddr=nullptr;
	return *this;
}
