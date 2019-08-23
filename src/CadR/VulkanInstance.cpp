#include <CadR/VulkanInstance.h>
#include <CadR/VulkanLibrary.h>

using namespace std;
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
	vkGetInstanceProcAddr                     =lib.vkGetInstanceProcAddr;
	vkDestroyInstance                         =getProcAddr<PFN_vkDestroyInstance                         >("vkDestroyInstance");
	vkGetPhysicalDeviceProperties             =getProcAddr<PFN_vkGetPhysicalDeviceProperties             >("vkGetPhysicalDeviceProperties");
	vkEnumeratePhysicalDevices                =getProcAddr<PFN_vkEnumeratePhysicalDevices                >("vkEnumeratePhysicalDevices");
	vkCreateDevice                            =getProcAddr<PFN_vkCreateDevice                            >("vkCreateDevice");
	vkGetDeviceProcAddr                       =getProcAddr<PFN_vkGetDeviceProcAddr                       >("vkGetDeviceProcAddr");
	vkEnumerateDeviceExtensionProperties      =getProcAddr<PFN_vkEnumerateDeviceExtensionProperties      >("vkEnumerateDeviceExtensionProperties");
	vkGetPhysicalDeviceSurfaceFormatsKHR      =getProcAddr<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR      >("vkGetPhysicalDeviceSurfaceFormatsKHR");
	vkGetPhysicalDeviceSurfacePresentModesKHR =getProcAddr<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR >("vkGetPhysicalDeviceSurfacePresentModesKHR");
	vkGetPhysicalDeviceQueueFamilyProperties  =getProcAddr<PFN_vkGetPhysicalDeviceQueueFamilyProperties  >("vkGetPhysicalDeviceQueueFamilyProperties");
	vkGetPhysicalDeviceSurfaceSupportKHR      =getProcAddr<PFN_vkGetPhysicalDeviceSurfaceSupportKHR      >("vkGetPhysicalDeviceSurfaceSupportKHR");
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


tuple<vk::PhysicalDevice,uint32_t,uint32_t> VulkanInstance::chooseDeviceAndQueueFamilies(vk::SurfaceKHR surface)
{
	// find compatible devices
	// (On Windows, all graphics adapters capable of monitor output are usually compatible devices.
	// On Linux X11 platform, only one graphics adapter is compatible device (the one that
	// contains the window).
	vector<vk::PhysicalDevice> deviceList=_instance.enumeratePhysicalDevices(*this);
	vector<tuple<vk::PhysicalDevice,uint32_t>> compatibleDevicesSingleQueue;
	vector<tuple<vk::PhysicalDevice,uint32_t,uint32_t>> compatibleDevicesTwoQueues;
	for(vk::PhysicalDevice pd:deviceList) {

		// skip devices without VK_KHR_swapchain
		vector<vk::ExtensionProperties> extensionList=pd.enumerateDeviceExtensionProperties(nullptr,*this);
		for(vk::ExtensionProperties& e:extensionList)
			if(strcmp(e.extensionName,"VK_KHR_swapchain")==0)
				goto swapchainSupported;
		continue;
		swapchainSupported:

		// skip devices without surface formats and presentation modes
		uint32_t formatCount;
		vk::createResultValue(
			pd.getSurfaceFormatsKHR(surface,&formatCount,nullptr,*this),
			VULKAN_HPP_NAMESPACE_STRING"::PhysicalDevice::getSurfaceFormatsKHR");
		uint32_t presentationModeCount;
		vk::createResultValue(
			pd.getSurfacePresentModesKHR(surface,&presentationModeCount,nullptr,*this),
			VULKAN_HPP_NAMESPACE_STRING"::PhysicalDevice::getSurfacePresentModesKHR");
		if(formatCount==0||presentationModeCount==0)
			continue;

		// select queues (for graphics rendering and for presentation)
		uint32_t graphicsQueueFamily=UINT32_MAX;
		uint32_t presentationQueueFamily=UINT32_MAX;
		vector<vk::QueueFamilyProperties> queueFamilyList=pd.getQueueFamilyProperties(*this);
		uint32_t i=0;
		for(auto it=queueFamilyList.begin(); it!=queueFamilyList.end(); it++,i++) {
			bool p=pd.getSurfaceSupportKHR(i,surface,*this)!=0;
			if(it->queueFlags&vk::QueueFlagBits::eGraphics) {
				if(p) {
					compatibleDevicesSingleQueue.emplace_back(pd,i);
					goto nextDevice;
				}
				if(graphicsQueueFamily==UINT32_MAX)
					graphicsQueueFamily=i;
			}
			else {
				if(p)
					if(presentationQueueFamily==UINT32_MAX)
						presentationQueueFamily=i;
			}
		}
		compatibleDevicesTwoQueues.emplace_back(pd,graphicsQueueFamily,presentationQueueFamily);
		nextDevice:;
	}

	// choose device
	if(compatibleDevicesSingleQueue.size()>0) {
		auto t=compatibleDevicesSingleQueue.front();
		return make_tuple(std::get<0>(t),std::get<1>(t),std::get<1>(t));
	}
	else if(compatibleDevicesTwoQueues.size()>0) {
		auto t=compatibleDevicesTwoQueues.front();
		return t;
	}
	else
		return make_tuple(vk::PhysicalDevice(),0,0);
}
