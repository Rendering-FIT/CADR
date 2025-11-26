#pragma once

#include <vulkan/vulkan.hpp>
#include <filesystem>

namespace CadR {


class CADR_EXPORT VulkanLibrary {
protected:
	void* _lib = nullptr;
	static const std::filesystem::path _defaultName;

public:

	static const std::filesystem::path& defaultName();

	VulkanLibrary();
	VulkanLibrary(bool load);
	VulkanLibrary(const std::filesystem::path& libPath);
	~VulkanLibrary();

	void load(const std::filesystem::path& libPath = defaultName());
	void unload();
	bool loaded() const;

	void* handle() const;
	void set(nullptr_t);

	VulkanLibrary(VulkanLibrary&& other) noexcept;
	VulkanLibrary& operator=(VulkanLibrary&& rhs) noexcept;

	template<typename T> T getProcAddr(const char* name) const;
	template<typename T> T getProcAddr(const std::string& name) const;

	uint32_t enumerateInstanceVersion() const;
	std::vector<vk::ExtensionProperties> enumerateInstanceExtensionProperties() const;
	std::vector<vk::LayerProperties> enumerateInstanceLayerProperties() const;

	auto getVkHeaderVersion() const { return VK_HEADER_VERSION; }
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
inline const std::filesystem::path& VulkanLibrary::defaultName()  { return _defaultName; }
inline VulkanLibrary::VulkanLibrary()  { vkGetInstanceProcAddr=nullptr; vkCreateInstance=nullptr; }
inline VulkanLibrary::VulkanLibrary(bool init) : VulkanLibrary()  { if(init) load(); }
inline VulkanLibrary::VulkanLibrary(const std::filesystem::path& libPath) : VulkanLibrary()  { load(libPath); }
inline VulkanLibrary::~VulkanLibrary()  { unload(); }

template<typename T> T VulkanLibrary::getProcAddr(const char* name) const  { return reinterpret_cast<T>(vkGetInstanceProcAddr(nullptr,name)); }
template<typename T> T VulkanLibrary::getProcAddr(const std::string& name) const  { return reinterpret_cast<T>(vkGetInstanceProcAddr(nullptr,name.c_str())); }

inline bool VulkanLibrary::loaded() const  { return _lib!=nullptr; }
inline void* VulkanLibrary::handle() const  { return _lib; }
inline void VulkanLibrary::set(nullptr_t)  { _lib=nullptr; }

inline uint32_t VulkanLibrary::enumerateInstanceVersion() const  { return (vkEnumerateInstanceVersion==nullptr) ? VK_API_VERSION_1_0 : vk::enumerateInstanceVersion(*this); }
inline std::vector<vk::ExtensionProperties> VulkanLibrary::enumerateInstanceExtensionProperties() const  { return vk::enumerateInstanceExtensionProperties(nullptr,*this); }
inline std::vector<vk::LayerProperties> VulkanLibrary::enumerateInstanceLayerProperties() const  { return vk::enumerateInstanceLayerProperties(*this); }


}
