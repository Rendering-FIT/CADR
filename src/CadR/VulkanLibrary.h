#pragma once

#include <CadR/CadR.h>
#include <CadR/Export.h>
#include <vulkan/vulkan.hpp>
#if _WIN32 // MSVC 2017 and 2019
#include <filesystem>
#else // gcc 7.4.0 (Ubuntu 18.04) does support path only as experimental
#include <experimental/filesystem>
namespace std { namespace filesystem { using std::experimental::filesystem::path; } }
#endif

namespace CadR {


class CADR_EXPORT VulkanLibrary final {
protected:
	void* _lib = nullptr;
	static const std::filesystem::path _defaultName;

public:

	static const std::filesystem::path& defaultName();

	VulkanLibrary();
	VulkanLibrary(const std::filesystem::path& libPath);
	~VulkanLibrary();

	void init(const std::filesystem::path& libPath = defaultName());
	void reset();
	bool initialized() const;

	VulkanLibrary(VulkanLibrary&& other) noexcept;
	VulkanLibrary& operator=(VulkanLibrary&& rhs) noexcept;

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
inline const std::filesystem::path& VulkanLibrary::defaultName()  { return _defaultName; }
inline VulkanLibrary::VulkanLibrary()  { vkGetInstanceProcAddr=nullptr; vkCreateInstance=nullptr; }
inline VulkanLibrary::VulkanLibrary(const std::filesystem::path& libPath)  { init(libPath); }
inline VulkanLibrary::~VulkanLibrary()  { if(!CadR::leakHandles()) reset(); }

template<typename T> T VulkanLibrary::getProcAddr(const char* name) const  { return reinterpret_cast<T>(vkGetInstanceProcAddr(nullptr,name)); }
template<typename T> T VulkanLibrary::getProcAddr(const std::string& name) const  { return reinterpret_cast<T>(vkGetInstanceProcAddr(nullptr,name.c_str())); }

inline bool VulkanLibrary::initialized() const  { return _lib!=nullptr; }
inline uint32_t VulkanLibrary::enumerateInstanceVersion() const  { return (vkEnumerateInstanceVersion==nullptr) ? VK_API_VERSION_1_0 : vk::enumerateInstanceVersion(*this); }
inline std::vector<vk::ExtensionProperties> VulkanLibrary::enumerateInstanceExtensionProperties() const  { return vk::enumerateInstanceExtensionProperties(nullptr,*this); }
inline std::vector<vk::LayerProperties> VulkanLibrary::enumerateInstanceLayerProperties() const  { return vk::enumerateInstanceLayerProperties(*this); }


}
