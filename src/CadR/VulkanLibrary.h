#pragma once

#include <vulkan/vulkan.hpp>
#include <experimental/filesystem>

namespace CadR {


class VulkanLibrary final {
protected:
	void* _lib = nullptr;
	static const std::experimental::filesystem::path _defaultPath;

public:

	static const std::experimental::filesystem::path& defaultPath();

	VulkanLibrary(nullptr_t);
	VulkanLibrary(const std::experimental::filesystem::path& libPath = defaultPath());
	~VulkanLibrary();

	void init(const std::experimental::filesystem::path& libPath = defaultPath());
	void reset();
	bool initialized() const;

	VulkanLibrary(VulkanLibrary&&);
	VulkanLibrary& operator=(VulkanLibrary&&);

	template<typename T> T getProcAddr(const char* name) const;
	template<typename T> T getProcAddr(const std::string& name) const;

	uint32_t enumerateInstanceVersion() const;
	std::vector<vk::ExtensionProperties> enumerateInstanceExtensionProperties() const;
	std::vector<vk::LayerProperties> enumerateInstanceLayerProperties() const;

	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
	PFN_vkCreateInstance vkCreateInstance;
	PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion;
	PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
	PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;

private:
	VulkanLibrary(const VulkanLibrary&) = default;  ///< Private copy contructor. Object copies not allowed. Only internal use.
	VulkanLibrary& operator=(const VulkanLibrary&) = default;  ///< Private copy assignment. Object copies not allowed. Only internal use.
};


// inline and template methods
inline const std::experimental::filesystem::path& VulkanLibrary::defaultPath()  { return _defaultPath; }
inline VulkanLibrary::VulkanLibrary(nullptr_t)  { vkGetInstanceProcAddr=nullptr; vkCreateInstance=nullptr; }
inline VulkanLibrary::VulkanLibrary(const std::experimental::filesystem::path& libPath)  { init(libPath); }
inline VulkanLibrary::~VulkanLibrary()  { reset(); }

template<typename T> T VulkanLibrary::getProcAddr(const char* name) const  { return reinterpret_cast<T>(vkGetInstanceProcAddr(nullptr,name)); }
template<typename T> T VulkanLibrary::getProcAddr(const std::string& name) const  { return reinterpret_cast<T>(vkGetInstanceProcAddr(nullptr,name.c_str())); }

inline bool VulkanLibrary::initialized() const  { return _lib!=nullptr; }
inline uint32_t VulkanLibrary::enumerateInstanceVersion() const  { return (vkEnumerateInstanceVersion==nullptr) ? VK_API_VERSION_1_0 : vk::enumerateInstanceVersion(*this); }
inline std::vector<vk::ExtensionProperties> VulkanLibrary::enumerateInstanceExtensionProperties() const  { return vk::enumerateInstanceExtensionProperties(nullptr,*this); }
inline std::vector<vk::LayerProperties> VulkanLibrary::enumerateInstanceLayerProperties() const  { return vk::enumerateInstanceLayerProperties(*this); }


}