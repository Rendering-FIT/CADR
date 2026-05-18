// SPDX-FileCopyrightText: 2019-2025 PCJohn (Jan Pečiva, peciva@fit.vut.cz)
//
// SPDX-License-Identifier: MIT

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


tuple<vk::PhysicalDevice, uint32_t, uint32_t> VulkanInstance::chooseDevice(
		vk::QueueFlags queueOperations,
		vk::SurfaceKHR presentationSurface,
		const std::string& nameFilter,
		const std::function<bool (VulkanInstance&, vk::PhysicalDevice)>& filterCallback,
		int index)
{
	// find compatible devices
	vector<vk::PhysicalDevice> deviceList = enumeratePhysicalDevices();
	vector<tuple<vk::PhysicalDevice, uint32_t, uint32_t, int>> compatibleDevices;
	compatibleDevices.reserve(deviceList.size());
	vk::PhysicalDeviceProperties deviceProperties;
	vector<tuple<uint32_t,uint32_t>> compatibleQueues;
	for(vk::PhysicalDevice pd : deviceList)
	{
		if(!presentationSurface)
		{
			// iterate queue families
			vector<vk::QueueFamilyProperties> queueFamilyList = getPhysicalDeviceQueueFamilyProperties(pd);
			for(uint32_t i=0, c=uint32_t(queueFamilyList.size()); i<c; i++) {

				// test for queue operations support (graphics, compute, etc.)
				if((queueFamilyList[i].queueFlags & queueOperations) == queueOperations) {
					compatibleQueues.emplace_back(i, i);
					break;
				}
			}
		}
		else
		{
			// skip devices without VK_KHR_swapchain
			vector<vk::ExtensionProperties> extensionList = enumerateDeviceExtensionProperties(pd);
			for(vk::ExtensionProperties& e : extensionList)
				if(strcmp(e.extensionName, "VK_KHR_swapchain") == 0)
					goto swapchainSupported;
			continue;
		swapchainSupported:;

			// select queues for submitting operations and for presentation
			uint32_t operationsQueueFamily = UINT32_MAX;
			uint32_t presentationQueueFamily = UINT32_MAX;
			vector<vk::QueueFamilyProperties> queueFamilyList = getPhysicalDeviceQueueFamilyProperties(pd);
			for(uint32_t i=0, c=uint32_t(queueFamilyList.size()); i<c; i++) {

				// test for presentation support
				if(pd.getSurfaceSupportKHR(i, presentationSurface, *this)) {

					// test for queue operations support (graphics, compute, etc.)
					if((queueFamilyList[i].queueFlags & queueOperations) == queueOperations) {
						// if operations and presentation are supported on the same queue,
						// we will use single queue
						compatibleQueues.emplace_back(i, i);
						goto nextDevice;
					}
					else
						// if only presentation is supported, we store the first such queue
						if(presentationQueueFamily == UINT32_MAX)
							presentationQueueFamily = i;
				}
				else {
					if((queueFamilyList[i].queueFlags & queueOperations) == queueOperations)
						// if only operations are supported on the queue,
						// we store the first such queue
						if(operationsQueueFamily == UINT32_MAX)
							operationsQueueFamily = i;
				}
			}

			if(operationsQueueFamily != UINT32_MAX && presentationQueueFamily != UINT32_MAX)
				// presentation and operations are supported on the different queues
				compatibleQueues.emplace_back(operationsQueueFamily, presentationQueueFamily);
		nextDevice:;
		}

		// get device properties
		deviceProperties = pd.getProperties(*this);

		// filter by device name
		if(!nameFilter.empty())
			if(string_view(deviceProperties.deviceName).find(nameFilter) == string::npos)
				continue;

		// filter by callback
		if(filterCallback)
			if(filterCallback(*this, pd) == false)
				continue;

		// evaluate score callback
		int score;
		constexpr const array deviceTypeScore = {
			10, // vk::PhysicalDeviceType::eOther         - lowest score
			40, // vk::PhysicalDeviceType::eIntegratedGpu - high score
			50, // vk::PhysicalDeviceType::eDiscreteGpu   - highest score
			30, // vk::PhysicalDeviceType::eVirtualGpu    - normal score
			20, // vk::PhysicalDeviceType::eCpu           - low score
		};
		int deviceType = int(deviceProperties.deviceType);
		if(deviceType >= 0 || deviceType < int(deviceTypeScore.size()))
			score = deviceTypeScore[deviceType];
		else
			score = 0;

		// append all compatible queue combinations
		// to compatible device list
		for(size_t i=0,c=compatibleQueues.size(); i<c; i++)
		{
			//  improve final score when operations and presentation queue families are the same
			auto [operationsQueueFamily, presentationQueueFamily] = compatibleQueues[i];
			int finalScore = score;
			if(operationsQueueFamily == presentationQueueFamily)
				finalScore++;

			compatibleDevices.emplace_back(pd, operationsQueueFamily, presentationQueueFamily, finalScore);
		}
		compatibleQueues.clear();
	}

	// sort devices
	sort(compatibleDevices.begin(), compatibleDevices.end(),
	     [](const decltype(compatibleDevices)::value_type& lhs, const decltype(compatibleDevices)::value_type& rhs)
		     { return get<3>(lhs) < get<3>(rhs); });

	// choose by index
	if(index >= 0 && index < decltype(index)(compatibleDevices.size())) {
		auto& d = compatibleDevices[index];
		return make_tuple(std::get<0>(d), std::get<1>(d), std::get<2>(d)); 
	}
	else
		return make_tuple(vk::PhysicalDevice(), 0, 0);
}


vector<string> VulkanInstance::getPhysicalDeviceNames(vk::QueueFlagBits queueOperations,
	vk::SurfaceKHR presentationSurface, const std::string& nameFilter,
	const function<bool (VulkanInstance&, vk::PhysicalDevice)>& filterCallback)
{
	// find compatible devices
	vector<vk::PhysicalDevice> deviceList = enumeratePhysicalDevices();
	vector<string> compatibleDevices;
	if(!presentationSurface)
	{
		// filter devices by supported operations
		for(vk::PhysicalDevice pd : deviceList)
		{
			// callback to filter out devices
			if(filterCallback)
				if(filterCallback(*this, pd) == false)
					continue;

			// iterate queue families
			vector<vk::QueueFamilyProperties> queueFamilyList = getPhysicalDeviceQueueFamilyProperties(pd);
			for(uint32_t i=0, c=uint32_t(queueFamilyList.size()); i<c; i++) {

				// test for queue operations support (graphics, compute, etc.)
				if((queueFamilyList[i].queueFlags & queueOperations) == queueOperations) {
					compatibleDevices.emplace_back(pd.getProperties(*this).deviceName.data());
					break;
				}
			}
		}
	}
	else
	{
		// filter devices by supported operations and presentation support
		for(vk::PhysicalDevice pd : deviceList)
		{
			// filter out devices
			if(filterCallback)
			{
				// callback
				// (do not forget to test for VK_KHR_swapchain inside filterCallback)
				if(filterCallback(*this, pd) == false)
					continue;
			}
			else
			{
				// skip devices without VK_KHR_swapchain
				vector<vk::ExtensionProperties> extensionList = enumerateDeviceExtensionProperties(pd);
				for(vk::ExtensionProperties& e : extensionList)
					if(strcmp(e.extensionName, "VK_KHR_swapchain") == 0)
						goto swapchainSupported;
				continue;
			swapchainSupported:;
			}

			// select queues for submitting operations and for presentation
			uint32_t operationsQueueFamily = UINT32_MAX;
			uint32_t presentationQueueFamily = UINT32_MAX;
			vector<vk::QueueFamilyProperties> queueFamilyList = getPhysicalDeviceQueueFamilyProperties(pd);
			for(uint32_t i=0, c=uint32_t(queueFamilyList.size()); i<c; i++) {

				// test for presentation support
				if(pd.getSurfaceSupportKHR(i, presentationSurface, *this)) {

					// test for queue operations support (graphics, compute, etc.)
					if((queueFamilyList[i].queueFlags & queueOperations) == queueOperations) {
						// if operations and presentation are supported on the same queue,
						// we will use single queue
						compatibleDevices.emplace_back(pd.getProperties(*this).deviceName.data());
						goto nextDevice;
					}
					else
						// if only presentation is supported, we store the first such queue
						if(presentationQueueFamily == UINT32_MAX)
							presentationQueueFamily = i;
				}
				else {
					if((queueFamilyList[i].queueFlags & queueOperations) == queueOperations)
						// if only operations are supported on the queue,
						// we store the first such queue
						if(operationsQueueFamily == UINT32_MAX)
							operationsQueueFamily = i;
				}
			}

			if(operationsQueueFamily != UINT32_MAX && presentationQueueFamily != UINT32_MAX)
				// presentation and operations are supported on the different queues
				compatibleDevices.emplace_back(pd.getProperties(*this).deviceName.data());
		nextDevice:;
		}
	}

	// filter physical devices
	if(!nameFilter.empty()) {
		decltype(compatibleDevices) filteredDevices;
		for(const string& s : compatibleDevices)
			if(nameFilter.find(s) != string::npos)
				filteredDevices.emplace_back(move(s));

		compatibleDevices.swap(filteredDevices);
	}

	return compatibleDevices;
}
