#pragma once

#include <vulkan/vulkan.hpp>
#include <functional>
#include <tuple>

namespace CadR {

class VulkanLibrary;


class CADR_EXPORT VulkanInstance {
protected:
	vk::Instance _instance;
	uint32_t _version;
public:

	std::tuple<vk::PhysicalDevice, uint32_t, uint32_t> chooseDevice(vk::QueueFlagBits queueOperations,
		vk::SurfaceKHR presentationSurface = {}, const std::string& nameFilter = "", int index = -1,
		const std::function<bool (VulkanInstance&, vk::PhysicalDevice)>& filterCallback = {});
	std::vector<std::string> getPhysicalDeviceNames(vk::QueueFlagBits queueOperations,
		vk::SurfaceKHR presentationSurface = {}, const std::string& nameFilter = "",
		const std::function<bool (VulkanInstance&, vk::PhysicalDevice)>& filterCallback = {});

	// constructors and destructor
	VulkanInstance();
	VulkanInstance(VulkanLibrary& lib, const vk::InstanceCreateInfo& createInfo);
	VulkanInstance(VulkanLibrary& lib, vk::Instance instance);
	VulkanInstance(VulkanLibrary& lib,
	               const char* applicationName = nullptr,
	               uint32_t applicationVersion = 0,
	               const char* engineName = nullptr,
	               uint32_t engineVersion = 0,
	               uint32_t apiVersion = VK_API_VERSION_1_0,
	               vk::ArrayProxy<const char*const> enabledLayers = nullptr,
	               vk::ArrayProxy<const char*const> enabledExtensions = nullptr);
	~VulkanInstance();

	// move constructor and move assignment
	// (copy constructor and copy assignment are private only)
	VulkanInstance(VulkanInstance&& other) noexcept;
	VulkanInstance& operator=(VulkanInstance&& rhs) noexcept;

	// initialization and finalization
	void create(VulkanLibrary& lib, const vk::InstanceCreateInfo& createInfo);
	void create(VulkanLibrary& lib,
	            const char* applicationName = nullptr,
	            uint32_t applicationVersion = 0,
	            const char* engineName = nullptr,
	            uint32_t engineVersion = 0,
	            uint32_t apiVersion = VK_API_VERSION_1_0,
	            vk::ArrayProxy<const char*const> enabledLayers = nullptr,
	            vk::ArrayProxy<const char*const> enabledExtensions = nullptr);
	void init(VulkanLibrary& lib, vk::Instance instance);
	bool initialized() const;
	void destroy();

	template<typename T> T getProcAddr(const char* name) const;
	template<typename T> T getProcAddr(const std::string& name) const;

	operator vk::Instance() const;
	operator VkInstance() const;
	explicit operator bool() const;
	bool operator!() const;

	uint32_t version() const;
	bool supportsVersion(uint32_t version) const;
	bool supportsVersion(uint32_t major, uint32_t minor, uint32_t patch=0) const;

	vk::Instance get() const;
	void set(nullptr_t);

	inline PFN_vkVoidFunction getProcAddr(const char* pName) const  { return _instance.getProcAddr(pName,*this); }
	inline auto getVkHeaderVersion() const { return VK_HEADER_VERSION; }
	inline void destroy(const vk::AllocationCallbacks* pAllocator) const noexcept  { _instance.destroy(pAllocator,*this); }
	inline vk::Result createDevice(vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo* pCreateInfo,const vk::AllocationCallbacks* pAllocator,vk::Device* pDevice) const  { return physicalDevice.createDevice(pCreateInfo,pAllocator,pDevice,*this); }
	inline vk::Result enumeratePhysicalDevices(uint32_t* pPhysicalDeviceCount,vk::PhysicalDevice* pPhysicalDevices) const  { return _instance.enumeratePhysicalDevices(pPhysicalDeviceCount,pPhysicalDevices,*this); }
	inline void getPhysicalDeviceProperties(vk::PhysicalDevice physicalDevice,vk::PhysicalDeviceProperties* pProperties) const noexcept  { physicalDevice.getProperties(pProperties,*this); }
	inline void getPhysicalDeviceProperties2(vk::PhysicalDevice physicalDevice,vk::PhysicalDeviceProperties2* pProperties) const noexcept  { physicalDevice.getProperties2(pProperties,*this); }
	inline vk::Result enumerateDeviceExtensionProperties(vk::PhysicalDevice physicalDevice,const char* pLayerName,uint32_t* pPropertyCount,vk::ExtensionProperties* pProperties) const  { return physicalDevice.enumerateDeviceExtensionProperties(pLayerName,pPropertyCount,pProperties,*this); }
	inline void getPhysicalDeviceFormatProperties(vk::PhysicalDevice physicalDevice,vk::Format format,vk::FormatProperties* pFormatProperties) const noexcept  { physicalDevice.getFormatProperties(format,pFormatProperties,*this); }
	inline void getPhysicalDeviceMemoryProperties(vk::PhysicalDevice physicalDevice,vk::PhysicalDeviceMemoryProperties* pMemoryProperties) const noexcept  { physicalDevice.getMemoryProperties(pMemoryProperties,*this); }
	inline void getPhysicalDeviceQueueFamilyProperties(vk::PhysicalDevice physicalDevice,uint32_t* pQueueFamilyPropertyCount,vk::QueueFamilyProperties* pQueueFamilyProperties) const noexcept  { physicalDevice.getQueueFamilyProperties(pQueueFamilyPropertyCount,pQueueFamilyProperties,*this); }
	inline void getPhysicalDeviceFeatures(vk::PhysicalDevice physicalDevice,vk::PhysicalDeviceFeatures* pFeatures) const  { physicalDevice.getFeatures(pFeatures,*this); }
	inline void getPhysicalDeviceFeatures2(vk::PhysicalDevice physicalDevice,vk::PhysicalDeviceFeatures2* pFeatures) const  { physicalDevice.getFeatures2(pFeatures,*this); }
	inline vk::Result getPhysicalDeviceCalibrateableTimeDomainsEXT(vk::PhysicalDevice physicalDevice,uint32_t* pTimeDomainCount,vk::TimeDomainEXT* pTimeDomains) const  { return physicalDevice.getCalibrateableTimeDomainsEXT(pTimeDomainCount,pTimeDomains,*this); }

#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
	inline void destroy(vk::Optional<const vk::AllocationCallbacks> allocator) const noexcept  { _instance.destroy(allocator,*this); }
	inline vk::ResultValueType<vk::Device>::type createDevice(vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator=nullptr) const  { return physicalDevice.createDevice(createInfo,allocator,*this); }
	template<typename Allocator=std::allocator<vk::PhysicalDevice>>
	typename vk::ResultValueType<std::vector<vk::PhysicalDevice,Allocator>>::type enumeratePhysicalDevices() const  { return _instance.enumeratePhysicalDevices(*this); }
# if VK_HEADER_VERSION>=159
	template<typename Allocator=std::allocator<vk::PhysicalDevice>>
	typename vk::ResultValueType<std::vector<vk::PhysicalDevice,Allocator>>::type enumeratePhysicalDevices(Allocator& vectorAllocator) const  { return _instance.enumeratePhysicalDevices(vectorAllocator,*this); }
# else
	template<typename Allocator=std::allocator<vk::PhysicalDevice>>
	typename vk::ResultValueType<std::vector<vk::PhysicalDevice,Allocator>>::type enumeratePhysicalDevices(Allocator const& vectorAllocator) const  { return _instance.enumeratePhysicalDevices(vectorAllocator,*this); }
# endif
	inline vk::PhysicalDeviceProperties getPhysicalDeviceProperties(vk::PhysicalDevice physicalDevice) const noexcept  { return physicalDevice.getProperties(*this); }
	inline vk::PhysicalDeviceProperties2 getPhysicalDeviceProperties2(vk::PhysicalDevice physicalDevice) const noexcept  { return physicalDevice.getProperties2(*this); }
	template<typename X,typename Y,typename ...Z>
	inline vk::StructureChain<X, Y, Z...> getPhysicalDeviceProperties2(vk::PhysicalDevice physicalDevice) const noexcept  { return physicalDevice.getProperties2<X,Y,Z...>(*this); }
	template<typename Allocator=std::allocator<vk::ExtensionProperties>>
	typename vk::ResultValueType<std::vector<vk::ExtensionProperties,Allocator>>::type enumerateDeviceExtensionProperties(vk::PhysicalDevice physicalDevice,vk::Optional<const std::string> layerName=nullptr) const  { return physicalDevice.enumerateDeviceExtensionProperties(layerName,*this); }
# if VK_HEADER_VERSION>=159
	template<typename Allocator=std::allocator<vk::ExtensionProperties>>
	typename vk::ResultValueType<std::vector<vk::ExtensionProperties,Allocator>>::type enumerateDeviceExtensionProperties(vk::PhysicalDevice physicalDevice,vk::Optional<const std::string> layerName,Allocator& vectorAllocator) const  { return physicalDevice.enumerateDeviceExtensionProperties(layerName,vectorAllocator,*this); }
# else
	template<typename Allocator=std::allocator<vk::ExtensionProperties>>
	typename vk::ResultValueType<std::vector<vk::ExtensionProperties,Allocator>>::type enumerateDeviceExtensionProperties(vk::PhysicalDevice physicalDevice,vk::Optional<const std::string> layerName,Allocator const& vectorAllocator) const  { return physicalDevice.enumerateDeviceExtensionProperties(layerName,vectorAllocator,*this); }
# endif
	inline vk::FormatProperties getPhysicalDeviceFormatProperties(vk::PhysicalDevice physicalDevice,vk::Format format) const noexcept  { return physicalDevice.getFormatProperties(format,*this); }
	inline vk::PhysicalDeviceMemoryProperties getPhysicalDeviceMemoryProperties(vk::PhysicalDevice physicalDevice) const noexcept  { return physicalDevice.getMemoryProperties(*this); }
	template<typename Allocator=std::allocator<vk::QueueFamilyProperties>>
	std::vector<vk::QueueFamilyProperties,Allocator> getPhysicalDeviceQueueFamilyProperties(vk::PhysicalDevice physicalDevice) const  { return physicalDevice.getQueueFamilyProperties(*this); }
# if VK_HEADER_VERSION>=159
	template<typename Allocator=std::allocator<vk::QueueFamilyProperties>>
	std::vector<vk::QueueFamilyProperties,Allocator> getPhysicalDeviceQueueFamilyProperties(vk::PhysicalDevice physicalDevice,Allocator& vectorAllocator) const  { return physicalDevice.getQueueFamilyProperties(vectorAllocator,*this); }
# else
	template<typename Allocator=std::allocator<vk::QueueFamilyProperties>>
	std::vector<vk::QueueFamilyProperties,Allocator> getPhysicalDeviceQueueFamilyProperties(vk::PhysicalDevice physicalDevice,Allocator const& vectorAllocator) const  { return physicalDevice.getQueueFamilyProperties(vectorAllocator,*this); }
# endif
	inline vk::PhysicalDeviceFeatures getPhysicalDeviceFeatures(vk::PhysicalDevice physicalDevice) const  { return physicalDevice.getFeatures(*this); }
	inline vk::PhysicalDeviceFeatures2 getPhysicalDeviceFeatures2(vk::PhysicalDevice physicalDevice) const  { return physicalDevice.getFeatures2(*this); }
	template<typename X,typename Y,typename ...Z>
	inline vk::StructureChain<X,Y,Z...> getPhysicalDeviceFeatures2(vk::PhysicalDevice physicalDevice) const  { return physicalDevice.getFeatures2<X,Y,Z...>(*this); }
	template<typename Allocator=std::allocator<vk::TimeDomainEXT>>
	inline typename vk::ResultValueType<std::vector<vk::TimeDomainEXT,Allocator>>::type getPhysicalDeviceCalibrateableTimeDomainsEXT(vk::PhysicalDevice physicalDevice) const  { return physicalDevice.getCalibrateableTimeDomainsEXT(*this); }
# if VK_HEADER_VERSION>=159
	template<typename Allocator=std::allocator<vk::TimeDomainEXT>>
	inline typename vk::ResultValueType<std::vector<vk::TimeDomainEXT,Allocator>>::type getPhysicalDeviceCalibrateableTimeDomainsEXT(vk::PhysicalDevice physicalDevice,Allocator& vectorAllocator) const  { return physicalDevice.getCalibrateableTimeDomainsEXT(*this); }
# else
	template<typename Allocator=std::allocator<vk::TimeDomainEXT>>
	inline typename vk::ResultValueType<std::vector<vk::TimeDomainEXT,Allocator>>::type getPhysicalDeviceCalibrateableTimeDomainsEXT(vk::PhysicalDevice physicalDevice,Allocator const& vectorAllocator) const  { return physicalDevice.getCalibrateableTimeDomainsEXT(*this); }
# endif
#endif

#ifndef VULKAN_HPP_NO_SMART_HANDLE
	template<typename Dispatch>
	typename vk::ResultValueType<vk::UniqueHandle<vk::Device,Dispatch>>::type createDeviceUnique(vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo& createInfo,Dispatch const &d) const  {
		vk::Device device;
		vk::Result result=static_cast<vk::Result>(vkCreateDevice(physicalDevice,reinterpret_cast<const VkDeviceCreateInfo*>(&createInfo),nullptr,reinterpret_cast<VkDevice*>(&device)));
		vk::ObjectDestroy<vk::NoParent,Dispatch> deleter(nullptr,d);
	#if VK_HEADER_VERSION<210  // change made on 2022-03-28 in VulkanHPP and went out in 1.3.210
		return vk::createResultValue<vk::Device,Dispatch>(result,device,"vk::PhysicalDevice::createDeviceUnique",deleter);
	#else
		resultCheck(result, "vk::PhysicalDevice::createDeviceUnique");
		return createResultValueType(result, vk::UniqueHandle<vk::Device, Dispatch>(device, deleter));
	#endif
	}
	template<typename Dispatch>
	typename vk::ResultValueType<vk::UniqueHandle<vk::Device,Dispatch>>::type createDeviceUnique(vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo& createInfo,vk::Optional<const vk::AllocationCallbacks> allocator,Dispatch const &d) const  {
		vk::Device device;
		vk::Result result=static_cast<vk::Result>(vkCreateDevice(physicalDevice,reinterpret_cast<const VkDeviceCreateInfo*>(&createInfo),reinterpret_cast<const VkAllocationCallbacks*>(static_cast<const vk::AllocationCallbacks*>(allocator)),reinterpret_cast<VkDevice*>(&device)));
		vk::ObjectDestroy<vk::NoParent,Dispatch> deleter(allocator,d);
	#if VK_HEADER_VERSION<210  // change made on 2022-03-28 in VulkanHPP and went out in 1.3.210
		return vk::createResultValue<vk::Device,Dispatch>(result,device,"vk::PhysicalDevice::createDeviceUnique",deleter);
	#else
		resultCheck(result, "vk::PhysicalDevice::createDeviceUnique");
		return createResultValueType(result, vk::UniqueHandle<vk::Device, Dispatch>(device, deleter));
	#endif
	}
#endif

	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
	PFN_vkDestroyInstance vkDestroyInstance;
	PFN_vkCreateDevice vkCreateDevice;
	PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
	PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
	PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
	PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2;
	PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
	PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties;
	PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
	PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
	PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
	PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2;
	PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT vkGetPhysicalDeviceCalibrateableTimeDomainsEXT;

private:
	VulkanInstance(const VulkanInstance&) = default;  ///< Private copy contructor. Object copies not allowed. Only internal use.
	VulkanInstance& operator=(const VulkanInstance&) = default;  ///< Private copy assignment. Object copies not allowed. Only internal use.
};


// inline and template methods
inline VulkanInstance::VulkanInstance() : _version(0)  { vkDestroyInstance=nullptr; vkCreateDevice=nullptr; vkGetDeviceProcAddr=nullptr; }
inline VulkanInstance::VulkanInstance(VulkanLibrary& lib, const vk::InstanceCreateInfo& createInfo)  { create(lib, createInfo); }
inline VulkanInstance::VulkanInstance(VulkanLibrary& lib, vk::Instance instance)  { init(lib, instance); }
inline VulkanInstance::VulkanInstance(VulkanLibrary& lib, const char* applicationName, uint32_t applicationVersion,
		const char* engineName, uint32_t engineVersion, uint32_t apiVersion,
		vk::ArrayProxy<const char*const> enabledLayers, vk::ArrayProxy<const char*const> enabledExtensions)
{
	vk::ApplicationInfo appInfo(applicationName, applicationVersion, engineName, engineVersion, apiVersion);
	vk::InstanceCreateInfo createInfo(vk::InstanceCreateFlags(), &appInfo, enabledLayers.size(), enabledLayers.data(), enabledExtensions.size(), enabledExtensions.data());
	create(lib, createInfo);
}
inline VulkanInstance::~VulkanInstance()  { destroy(); }

inline void VulkanInstance::create(VulkanLibrary& lib, const char* applicationName, uint32_t applicationVersion,
		const char* engineName, uint32_t engineVersion, uint32_t apiVersion,
		vk::ArrayProxy<const char*const> enabledLayers, vk::ArrayProxy<const char*const> enabledExtensions)
{
	vk::ApplicationInfo appInfo(applicationName, applicationVersion, engineName, engineVersion, apiVersion);
	vk::InstanceCreateInfo createInfo(vk::InstanceCreateFlags(), &appInfo, enabledLayers.size(), enabledLayers.data(), enabledExtensions.size(), enabledExtensions.data());
	create(lib, createInfo);
}
inline bool VulkanInstance::initialized() const  { return _instance.operator bool(); }
template<typename T> T VulkanInstance::getProcAddr(const char* name) const  { return reinterpret_cast<T>(_instance.getProcAddr(name, *this)); }
template<typename T> T VulkanInstance::getProcAddr(const std::string& name) const  { return reinterpret_cast<T>(_instance.getProcAddr(name, *this)); }

inline VulkanInstance::operator vk::Instance() const  { return _instance; }
inline VulkanInstance::operator VkInstance() const  { return _instance; }
inline VulkanInstance::operator bool() const  { return _instance.operator bool(); }
inline bool VulkanInstance::operator!() const  { return _instance.operator!(); }
inline uint32_t VulkanInstance::version() const  { return _version; }
inline bool VulkanInstance::supportsVersion(uint32_t version) const  { return _version >= version; }
inline bool VulkanInstance::supportsVersion(uint32_t major,uint32_t minor,uint32_t patch) const  { return _version >= VK_MAKE_VERSION(major,minor,patch); }
inline vk::Instance VulkanInstance::get() const  { return _instance; }
inline void VulkanInstance::set(nullptr_t)  { _instance = nullptr; }


}
