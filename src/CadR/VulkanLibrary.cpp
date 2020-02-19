#include <CadR/VulkanLibrary.h>
#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN  // this reduces win32 headers default namespace pollution
# include <windows.h>
#else
# include <dlfcn.h>
#endif

using namespace std;
using namespace CadR;

#ifdef _WIN32
const filesystem::path VulkanLibrary::_defaultName = "vulkan-1.dll";
#else
const filesystem::path VulkanLibrary::_defaultName = "libvulkan.so.1";
#endif


void VulkanLibrary::init(const std::filesystem::path& libPath)
{
	// avoid multiple initialization attempts
	if(_lib)
		throw std::runtime_error("Multiple initialization attempts.");

	// load library
	// and get vkGetInstanceProcAddr pointer
#ifdef _WIN32
	_lib=reinterpret_cast<void*>(LoadLibraryW(libPath.native().c_str()));
	if(_lib==nullptr)
		throw std::runtime_error("Can not open \""+libPath.string()+"\".");
	vkGetInstanceProcAddr=PFN_vkGetInstanceProcAddr(GetProcAddress(reinterpret_cast<HMODULE>(_lib),"vkGetInstanceProcAddr"));
#else
	_lib=dlopen(libPath.native().c_str(),RTLD_NOW);
	if(_lib==nullptr)
		throw std::runtime_error("Can not open \""+libPath.native()+"\".");
	vkGetInstanceProcAddr=PFN_vkGetInstanceProcAddr(dlsym(_lib,"vkGetInstanceProcAddr"));
#endif
	if(vkGetInstanceProcAddr==nullptr)
		throw std::runtime_error("Can not retrieve vkGetInstanceProcAddr function pointer out of \"libvulkan.so.1\".");

	// function pointers available without vk::Instance
	vkCreateInstance=getProcAddr<PFN_vkCreateInstance>("vkCreateInstance");
	if(vkCreateInstance==nullptr)
		throw std::runtime_error("Can not retrieve vkCreateInstance function pointer out of \"libvulkan.so.1\".");
	vkEnumerateInstanceVersion=getProcAddr<PFN_vkEnumerateInstanceVersion>("vkEnumerateInstanceVersion");
	vkEnumerateInstanceExtensionProperties=getProcAddr<PFN_vkEnumerateInstanceExtensionProperties>("vkEnumerateInstanceExtensionProperties");
	vkEnumerateInstanceLayerProperties=getProcAddr<PFN_vkEnumerateInstanceLayerProperties>("vkEnumerateInstanceLayerProperties");
}


void VulkanLibrary::reset()
{
	if(_lib) {

		// release Vulkan library
#ifdef _WIN32
		FreeLibrary(reinterpret_cast<HMODULE>(_lib));
#else
		dlclose(_lib);
#endif
		_lib=nullptr;

		// set the most important pointers to null
		vkGetInstanceProcAddr=nullptr;
		vkCreateInstance=nullptr;
	}
}


VulkanLibrary::VulkanLibrary(VulkanLibrary&& other) noexcept
	: VulkanLibrary(other)
{
	other._lib=nullptr;
	other.vkGetInstanceProcAddr=nullptr;
	other.vkCreateInstance=nullptr;
}


VulkanLibrary& VulkanLibrary::operator=(VulkanLibrary&& other) noexcept
{
	if(_lib)
		reset();

	*this=other;
	other._lib=nullptr;
	other.vkGetInstanceProcAddr=nullptr;
	other.vkCreateInstance=nullptr;
	return *this;
}
