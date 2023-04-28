#include <CadR/VulkanInstance.h>
#include <CadR/VulkanLibrary.h>
#include <tuple>

using namespace std;
using namespace CadR;


void VulkanInstance::init(VulkanLibrary& lib,vk::Instance instance)
{
	if(_instance)
		destroy();

	_instance=instance;
	_version=lib.enumerateInstanceVersion();

	vkGetInstanceProcAddr                      =lib.vkGetInstanceProcAddr;
	vkDestroyInstance                          =getProcAddr<PFN_vkDestroyInstance                          >("vkDestroyInstance");
	vkGetPhysicalDeviceProperties              =getProcAddr<PFN_vkGetPhysicalDeviceProperties              >("vkGetPhysicalDeviceProperties");
	vkGetPhysicalDeviceProperties2             =getProcAddr<PFN_vkGetPhysicalDeviceProperties2             >("vkGetPhysicalDeviceProperties2");
	vkEnumeratePhysicalDevices                 =getProcAddr<PFN_vkEnumeratePhysicalDevices                 >("vkEnumeratePhysicalDevices");
	vkCreateDevice                             =getProcAddr<PFN_vkCreateDevice                             >("vkCreateDevice");
	vkGetDeviceProcAddr                        =getProcAddr<PFN_vkGetDeviceProcAddr                        >("vkGetDeviceProcAddr");
	vkEnumerateDeviceExtensionProperties       =getProcAddr<PFN_vkEnumerateDeviceExtensionProperties       >("vkEnumerateDeviceExtensionProperties");
	vkGetPhysicalDeviceSurfaceFormatsKHR       =getProcAddr<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR       >("vkGetPhysicalDeviceSurfaceFormatsKHR");
	vkGetPhysicalDeviceFormatProperties        =getProcAddr<PFN_vkGetPhysicalDeviceFormatProperties        >("vkGetPhysicalDeviceFormatProperties");
	vkGetPhysicalDeviceMemoryProperties        =getProcAddr<PFN_vkGetPhysicalDeviceMemoryProperties        >("vkGetPhysicalDeviceMemoryProperties");
	vkGetPhysicalDeviceSurfacePresentModesKHR  =getProcAddr<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR  >("vkGetPhysicalDeviceSurfacePresentModesKHR");
	vkGetPhysicalDeviceQueueFamilyProperties   =getProcAddr<PFN_vkGetPhysicalDeviceQueueFamilyProperties   >("vkGetPhysicalDeviceQueueFamilyProperties");
	vkGetPhysicalDeviceSurfaceSupportKHR       =getProcAddr<PFN_vkGetPhysicalDeviceSurfaceSupportKHR       >("vkGetPhysicalDeviceSurfaceSupportKHR");
	vkGetPhysicalDeviceFeatures                =getProcAddr<PFN_vkGetPhysicalDeviceFeatures                >("vkGetPhysicalDeviceFeatures");
	vkGetPhysicalDeviceFeatures2               =getProcAddr<PFN_vkGetPhysicalDeviceFeatures2               >("vkGetPhysicalDeviceFeatures2");
	vkGetPhysicalDeviceCalibrateableTimeDomainsEXT=getProcAddr<PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT>("vkGetPhysicalDeviceCalibrateableTimeDomainsEXT");
}


void VulkanInstance::create(VulkanLibrary& lib,const vk::InstanceCreateInfo& createInfo)
{
	if(_instance)
		destroy();

	// make sure lib is initialized
	if(!lib.loaded() || lib.vkGetInstanceProcAddr==nullptr)
		throw std::runtime_error("VulkanLibrary class was not initialized.");

	// create instance
	// (Handle the special case of non-1.0 Vulkan version request on Vulkan 1.0 system that throws. See vkCreateInstance() documentation.)
	vk::Instance instance;
	if(createInfo.pApplicationInfo && createInfo.pApplicationInfo->apiVersion!=VK_API_VERSION_1_0)
	{
		if(lib.enumerateInstanceVersion()==VK_API_VERSION_1_0)
		{
			// replace requested Vulkan version by 1.0 to avoid throwing the exception
			vk::ApplicationInfo appInfo(*createInfo.pApplicationInfo);
			appInfo.apiVersion=VK_API_VERSION_1_0;
			vk::InstanceCreateInfo createInfo2(createInfo);
			createInfo2.pApplicationInfo=&appInfo;
			instance=vk::createInstance(createInfo2,nullptr,lib);
		}
		else
			instance=vk::createInstance(createInfo,nullptr,lib);
	}
	else
		instance=vk::createInstance(createInfo,nullptr,lib);

	init(lib,instance);
}


void VulkanInstance::destroy()
{
	if(_instance) {
		_instance.destroy(nullptr,*this);
		_instance=nullptr;
		vkDestroyInstance=nullptr;
		vkCreateDevice=nullptr;
		vkGetDeviceProcAddr=nullptr;
	}
}


VulkanInstance::VulkanInstance(VulkanInstance&& other) noexcept
	: VulkanInstance(other)
{
	other._instance=nullptr;
	other.vkDestroyInstance=nullptr;
	other.vkCreateDevice=nullptr;
	other.vkGetDeviceProcAddr=nullptr;
}


VulkanInstance& VulkanInstance::operator=(VulkanInstance&& other) noexcept
{
	if(_instance)
		destroy();

	*this=other;
	other._instance=nullptr;
	other.vkDestroyInstance=nullptr;
	other.vkCreateDevice=nullptr;
	other.vkGetDeviceProcAddr=nullptr;
	return *this;
}


tuple<vk::PhysicalDevice, uint32_t, uint32_t> VulkanInstance::chooseDeviceForRenderingAndPresentation(vk::SurfaceKHR surface)
{
	// find compatible devices
	// (On Windows, all graphics adapters capable of monitor output are usually compatible devices.
	// On Linux X11 platform, only one graphics adapter is compatible device (the one that
	// contains the window).
	vector<vk::PhysicalDevice> deviceList = enumeratePhysicalDevices();
	vector<tuple<vk::PhysicalDevice, uint32_t, uint32_t, vk::PhysicalDeviceProperties>> compatibleDevices;
	for(vk::PhysicalDevice pd : deviceList) {

		// skip devices without VK_KHR_swapchain
		vector<vk::ExtensionProperties> extensionList = enumerateDeviceExtensionProperties(pd);
		for(vk::ExtensionProperties& e : extensionList)
			if(strcmp(e.extensionName, "VK_KHR_swapchain") == 0)
				goto swapchainSupported;
		continue;
		swapchainSupported:

		// select queues for graphics rendering and for presentation
		uint32_t graphicsQueueFamily = UINT32_MAX;
		uint32_t presentationQueueFamily = UINT32_MAX;
		vector<vk::QueueFamilyProperties> queueFamilyList = getPhysicalDeviceQueueFamilyProperties(pd);
		for(uint32_t i=0, c=uint32_t(queueFamilyList.size()); i<c; i++) {

			// test for presentation support
			if(pd.getSurfaceSupportKHR(i, surface, *this)) {

				// test for graphics operations support
				if(queueFamilyList[i].queueFlags & vk::QueueFlagBits::eGraphics) {
					// if presentation and graphics operations are supported on the same queue,
					// we will use single queue
					compatibleDevices.emplace_back(pd, i, i, pd.getProperties(*this));
					goto nextDevice;
				}
				else
					// if only presentation is supported, we store the first such queue
					if(presentationQueueFamily == UINT32_MAX)
						presentationQueueFamily = i;
			}
			else {
				if(queueFamilyList[i].queueFlags & vk::QueueFlagBits::eGraphics)
					// if only graphics operations are supported, we store the first such queue
					if(graphicsQueueFamily == UINT32_MAX)
						graphicsQueueFamily = i;
			}
		}

		if(graphicsQueueFamily != UINT32_MAX && presentationQueueFamily != UINT32_MAX)
			// presentation and graphics operations are supported on the different queues
			compatibleDevices.emplace_back(pd, graphicsQueueFamily, presentationQueueFamily, pd.getProperties(*this));
		nextDevice:;
	}

	// choose the best device
	auto bestDevice = compatibleDevices.begin();
	if(bestDevice == compatibleDevices.end())
		return make_tuple(vk::PhysicalDevice(), 0, 0);
	constexpr const array deviceTypeScore = {
		10, // vk::PhysicalDeviceType::eOther         - lowest score
		40, // vk::PhysicalDeviceType::eIntegratedGpu - high score
		50, // vk::PhysicalDeviceType::eDiscreteGpu   - highest score
		30, // vk::PhysicalDeviceType::eVirtualGpu    - normal score
		20, // vk::PhysicalDeviceType::eCpu           - low score
		10, // unknown vk::PhysicalDeviceType
	};
	int bestScore = deviceTypeScore[clamp(int(std::get<3>(*bestDevice).deviceType), 0, int(deviceTypeScore.size())-1)];
	if(std::get<1>(*bestDevice) == std::get<2>(*bestDevice))
		bestScore++;
	for(auto it=compatibleDevices.begin()+1; it!=compatibleDevices.end(); it++) {
		int score = deviceTypeScore[clamp(int(std::get<3>(*it).deviceType), 0, int(deviceTypeScore.size())-1)];
		if(std::get<1>(*it) == std::get<2>(*it))
			score++;
		if(score > bestScore) {
			bestDevice = it;
			bestScore = score;
		}
	}
	return make_tuple(std::get<0>(*bestDevice), std::get<1>(*bestDevice), std::get<2>(*bestDevice)); 
}


tuple<vk::PhysicalDevice, uint32_t, uint32_t> VulkanInstance::chooseDeviceForOffscreenRendering()
{
	// find compatible devices
	vector<vk::PhysicalDevice> deviceList = enumeratePhysicalDevices();
	vector<tuple<vk::PhysicalDevice, uint32_t, vk::PhysicalDeviceProperties>> compatibleDevices;
	for(vk::PhysicalDevice pd : deviceList) {

		// select queues for graphics rendering
		vector<vk::QueueFamilyProperties> queueFamilyList = getPhysicalDeviceQueueFamilyProperties(pd);
		for(uint32_t i=0, c=uint32_t(queueFamilyList.size()); i<c; i++) {

			// test for graphics operations support
			if(queueFamilyList[i].queueFlags & vk::QueueFlagBits::eGraphics)
				compatibleDevices.emplace_back(pd, i, pd.getProperties(*this));

		}

	}

	// choose the best device
	auto bestDevice = compatibleDevices.begin();
	if(bestDevice == compatibleDevices.end())
		return make_tuple(vk::PhysicalDevice(), 0, 0);
	constexpr const array deviceTypeScore = {
		10, // vk::PhysicalDeviceType::eOther         - lowest score
		40, // vk::PhysicalDeviceType::eIntegratedGpu - high score
		50, // vk::PhysicalDeviceType::eDiscreteGpu   - highest score
		30, // vk::PhysicalDeviceType::eVirtualGpu    - normal score
		20, // vk::PhysicalDeviceType::eCpu           - low score
		10, // unknown vk::PhysicalDeviceType
	};
	int bestScore = deviceTypeScore[clamp(int(std::get<2>(*bestDevice).deviceType), 0, int(deviceTypeScore.size())-1)];
	for(auto it=compatibleDevices.begin()+1; it!=compatibleDevices.end(); it++) {
		int score = deviceTypeScore[clamp(int(std::get<2>(*it).deviceType), 0, int(deviceTypeScore.size())-1)];
		if(score > bestScore) {
			bestDevice = it;
			bestScore = score;
		}
	}
	return make_tuple(std::get<0>(*bestDevice), std::get<1>(*bestDevice), std::get<1>(*bestDevice)); 
}
