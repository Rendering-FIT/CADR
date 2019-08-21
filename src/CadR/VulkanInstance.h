#pragma once

#include <vulkan/vulkan.hpp>

namespace CadR {

class VulkanLibrary;


class VulkanInstance final {
protected:
	vk::Instance _instance;
public:

	VulkanInstance();
	VulkanInstance(VulkanLibrary& lib,const vk::InstanceCreateInfo& createInfo);
	VulkanInstance(VulkanLibrary& lib,
	               const char* applicationName = nullptr,
	               uint32_t applicationVersion = 0,
	               const char* engineName = nullptr,
	               uint32_t engineVersion = 0,
	               uint32_t apiVersion = 0,
	               const vk::ArrayProxy<const char*> enabledLayers = nullptr,
	               const vk::ArrayProxy<const char*> enabledExtensions = nullptr);
	~VulkanInstance();

	void init(VulkanLibrary& lib,const vk::InstanceCreateInfo& createInfo);
	void reset();

	VulkanInstance(VulkanInstance&&);
	VulkanInstance& operator=(VulkanInstance&&);

	template<typename T> T getProcAddr(const VulkanLibrary& lib,const char* name) const;
	template<typename T> T getProcAddr(const VulkanLibrary& lib,const std::string& name) const;

	operator vk::Instance() const;
	explicit operator bool() const;
	bool operator!() const;

	const vk::Instance* operator->() const;
	vk::Instance* operator->();
	const vk::Instance& operator*() const;
	vk::Instance& operator*();
	const vk::Instance& get() const;
	vk::Instance& get();

	PFN_vkDestroyInstance vkDestroyInstance;
	PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
	PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
	PFN_vkCreateDevice vkCreateDevice;
	PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;

private:
	VulkanInstance(const VulkanInstance&) = default;  ///< Private copy contructor. Object copies not allowed. Only internal use.
	VulkanInstance& operator=(const VulkanInstance&) = default;  ///< Private copy assignment. Object copies not allowed. Only internal use.
};


// inline and template methods
inline VulkanInstance::VulkanInstance()  { vkDestroyInstance=nullptr; vkCreateDevice=nullptr; vkGetDeviceProcAddr=nullptr; }
inline VulkanInstance::VulkanInstance(VulkanLibrary& lib,const vk::InstanceCreateInfo& createInfo)  { init(lib,createInfo); }
inline VulkanInstance::VulkanInstance(VulkanLibrary& lib,const char* applicationName,uint32_t applicationVersion,
		const char* engineName,uint32_t engineVersion,uint32_t apiVersion,
		const vk::ArrayProxy<const char*> enabledLayers,const vk::ArrayProxy<const char*> enabledExtensions)
	{	vk::ApplicationInfo appInfo(applicationName,applicationVersion,engineName,engineVersion,apiVersion);
		vk::InstanceCreateInfo createInfo(vk::InstanceCreateFlags(),&appInfo,enabledLayers.size(),enabledLayers.data(),enabledExtensions.size(),enabledExtensions.data());
		init(lib,createInfo); }
inline VulkanInstance::~VulkanInstance()  { reset(); }

template<typename T> T VulkanInstance::getProcAddr(const VulkanLibrary& lib,const char* name) const  { return reinterpret_cast<T>(_instance.getProcAddr(name,lib)); }
template<typename T> T VulkanInstance::getProcAddr(const VulkanLibrary& lib,const std::string& name) const  { return reinterpret_cast<T>(_instance.getProcAddr(name,lib)); }

inline VulkanInstance::operator vk::Instance() const  { return _instance; }
inline VulkanInstance::operator bool() const  { return _instance.operator bool(); }
inline bool VulkanInstance::operator!() const  { return _instance.operator!(); }
inline const vk::Instance* VulkanInstance::operator->() const  { return &_instance; }
inline vk::Instance* VulkanInstance::operator->()  { return &_instance; }
inline const vk::Instance& VulkanInstance::operator*() const  { return _instance; }
inline vk::Instance& VulkanInstance::operator*()  { return _instance; }
inline const vk::Instance& VulkanInstance::get() const  { return _instance; }
inline vk::Instance& VulkanInstance::get()  { return _instance; }


}
