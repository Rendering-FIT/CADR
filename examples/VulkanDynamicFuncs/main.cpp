#include <vulkan/vulkan.hpp>
#include <iostream>
#include <stddef.h>
#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#else
# include <dlfcn.h>
#endif

using namespace std;


class VulkanLibrary {
protected:
#ifdef _WIN32
	HMODULE _lib = nullptr;
#else
	void* _lib = nullptr;
#endif
public:

	VulkanLibrary() = default;
	void init();
	~VulkanLibrary();

	VulkanLibrary(VulkanLibrary&&);
	VulkanLibrary(const VulkanLibrary&) = delete;
	VulkanLibrary& operator=(VulkanLibrary&&);
	VulkanLibrary& operator=(const VulkanLibrary&) = delete;

	void reset();

	struct Funcs {
		auto getVkHeaderVersion() const { return VK_HEADER_VERSION; }
		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
		PFN_vkCreateInstance vkCreateInstance = nullptr;
	} funcs;

};

VulkanLibrary::VulkanLibrary(VulkanLibrary&& other) : _lib(other._lib), funcs(other.funcs)  { other._lib=nullptr; }
VulkanLibrary& VulkanLibrary::operator=(VulkanLibrary&& other)  { reset(); _lib=other._lib; other._lib=nullptr; funcs=other.funcs; return *this; }

void VulkanLibrary::reset()
{
	if(_lib) {
		// release Vulkan library
#ifdef _WIN32
		FreeLibrary(_lib);
#else
		dlclose(_lib);
#endif
		_lib=nullptr;

		// set the most important pointers to null
		funcs.vkGetInstanceProcAddr=nullptr;
		funcs.vkCreateInstance=nullptr;
	}
}

VulkanLibrary::~VulkanLibrary()
{
	reset();
}

void VulkanLibrary::init()
{
	// avoid multiple initializations
	if(_lib)
		throw std::runtime_error("VulkanLibrary::init() called multiple times.");

	// load library
	// and get vkGetInstanceProcAddr pointer
#ifdef _WIN32
	_lib=LoadLibraryA("vulkan-1.dll");
	if(_lib==nullptr)
		throw std::runtime_error("Can not open \"vulkan-1.dll\".");
	funcs.vkGetInstanceProcAddr=PFN_vkGetInstanceProcAddr(GetProcAddress(_lib,"vkGetInstanceProcAddr"));
#else
	_lib=dlopen("libvulkan.so.1",RTLD_NOW);
	if(_lib==nullptr)
		throw std::runtime_error("Can not open \"libvulkan.so.1\".");
	funcs.vkGetInstanceProcAddr=PFN_vkGetInstanceProcAddr(dlsym(_lib,"vkGetInstanceProcAddr"));
#endif
	if(funcs.vkGetInstanceProcAddr==nullptr)
		throw std::runtime_error("Can not retrieve vkGetInstanceProcAddr function pointer out of \"libvulkan.so.1\".");

	// load function pointers
	vk::Instance nullInstance;
	funcs.vkCreateInstance=PFN_vkCreateInstance(nullInstance.getProcAddr("vkCreateInstance",this->funcs));
	if(funcs.vkCreateInstance==nullptr)
		throw std::runtime_error("Can not retrieve vkCreateInstance function pointer out of \"libvulkan.so.1\".");
}


class VulkanInstance {
protected:
	vk::Instance _instance;
public:

	VulkanInstance() = default;
	VulkanInstance(VulkanLibrary& lib,const vk::InstanceCreateInfo& createInfo)  { init(lib,createInfo); }
	void init(VulkanLibrary& lib,const vk::InstanceCreateInfo& createInfo);
	~VulkanInstance();

	VulkanInstance(VulkanInstance&&);
	VulkanInstance(const VulkanInstance&) = delete;
	VulkanInstance& operator=(VulkanInstance&&);
	VulkanInstance& operator=(const VulkanInstance&) = delete;

	operator vk::Instance() const;
	explicit operator bool() const;
	bool operator!() const;

	const vk::Instance* operator->() const;
	vk::Instance* operator->();
	const vk::Instance& operator*() const;
	vk::Instance& operator*();
	const vk::Instance& get() const;
	vk::Instance& get();

	void reset();

	struct Funcs {
		auto getVkHeaderVersion() const { return VK_HEADER_VERSION; }
		PFN_vkDestroyInstance vkDestroyInstance = nullptr;
		PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = nullptr;
		PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = nullptr;
		PFN_vkCreateDevice vkCreateDevice = nullptr;
		PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = nullptr;
	} funcs;

};

VulkanInstance::VulkanInstance(VulkanInstance&& other) : _instance(other._instance), funcs(other.funcs)  { other._instance=nullptr; }
VulkanInstance& VulkanInstance::operator=(VulkanInstance&& other)  { _instance=other._instance; other._instance=nullptr; funcs=other.funcs; return *this; }
VulkanInstance::operator vk::Instance() const  { return _instance; }
VulkanInstance::operator bool() const  { return _instance.operator bool(); }
bool VulkanInstance::operator!() const  { return _instance.operator!(); }
const vk::Instance* VulkanInstance::operator->() const  { return &_instance; }
vk::Instance* VulkanInstance::operator->()  { return &_instance; }
const vk::Instance& VulkanInstance::operator*() const  { return _instance; }
vk::Instance& VulkanInstance::operator*()  { return _instance; }
const vk::Instance& VulkanInstance::get() const  { return _instance; }
vk::Instance& VulkanInstance::get()  { return _instance; }

void VulkanInstance::reset()
{
	if(_instance) {
		_instance.destroy(nullptr,this->funcs);
		_instance=nullptr;
	}
}

VulkanInstance::~VulkanInstance()
{
	reset();
}

void VulkanInstance::init(VulkanLibrary& lib,const vk::InstanceCreateInfo& createInfo)
{
	// avoid multiple initialization attempts
	if(_instance)
		throw std::runtime_error("Multiple initialization attempts.");

	// make sure lib is initialized
	if(lib.funcs.vkGetInstanceProcAddr==nullptr)
		throw std::runtime_error("VulkanLibrary class was not initialized.");

	// create instance
	_instance=vk::createInstance(createInfo,nullptr,lib.funcs);

	// get function pointers
	funcs.vkDestroyInstance=PFN_vkDestroyInstance(_instance.getProcAddr("vkDestroyInstance",lib.funcs));
	funcs.vkGetPhysicalDeviceProperties=PFN_vkGetPhysicalDeviceProperties(_instance.getProcAddr("vkGetPhysicalDeviceProperties",lib.funcs));
	funcs.vkEnumeratePhysicalDevices=PFN_vkEnumeratePhysicalDevices(_instance.getProcAddr("vkEnumeratePhysicalDevices",lib.funcs));
	funcs.vkCreateDevice=PFN_vkCreateDevice(_instance.getProcAddr("vkCreateDevice",lib.funcs));
	funcs.vkGetDeviceProcAddr=PFN_vkGetDeviceProcAddr(_instance.getProcAddr("vkGetDeviceProcAddr",lib.funcs));
}


class VulkanDevice {
protected:
	vk::Device _device;
public:

	VulkanDevice() = default;
	VulkanDevice(VulkanInstance& instance,vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo& createInfo);
	void init(VulkanInstance& instance,vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo& createInfo);
	~VulkanDevice();

	VulkanDevice(VulkanDevice&&);
	VulkanDevice(const VulkanDevice&) = delete;
	VulkanDevice& operator=(VulkanDevice&&);
	VulkanDevice& operator=(const VulkanDevice&) = delete;

	operator vk::Device() const;
	explicit operator bool() const;
	bool operator!() const;

	const vk::Device* operator->() const;
	vk::Device* operator->();
	const vk::Device& operator*() const;
	vk::Device& operator*();
	const vk::Device& get() const;
	vk::Device& get();
	void reset();

	struct Funcs {
		auto getVkHeaderVersion() const { return VK_HEADER_VERSION; }
		PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = nullptr;
		PFN_vkDestroyDevice vkDestroyDevice = nullptr;
		PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
		PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool;
	} funcs;

};

VulkanDevice::VulkanDevice(VulkanInstance& instance,vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo& createInfo)  { init(instance,physicalDevice,createInfo); }
VulkanDevice::VulkanDevice(VulkanDevice&& other) : _device(other._device), funcs(other.funcs)  { other._device=nullptr; }
VulkanDevice& VulkanDevice::operator=(VulkanDevice&& other)  { _device=other._device; other._device=nullptr; funcs=other.funcs; return *this; }
VulkanDevice::operator vk::Device() const  { return _device; }
VulkanDevice::operator bool() const  { return _device.operator bool(); }
bool VulkanDevice::operator!() const  { return _device.operator!(); }
const vk::Device* VulkanDevice::operator->() const  { return &_device; }
vk::Device* VulkanDevice::operator->()  { return &_device; }
const vk::Device& VulkanDevice::operator*() const  { return _device; }
vk::Device& VulkanDevice::operator*()  { return _device; }
const vk::Device& VulkanDevice::get() const  { return _device; }
vk::Device& VulkanDevice::get()  { return _device; }

void VulkanDevice::reset()
{
	if(_device) {
		_device.destroy(nullptr,this->funcs);
		_device=nullptr;
	}
}

VulkanDevice::~VulkanDevice()
{
	// destroy device
	reset();
}

void VulkanDevice::init(VulkanInstance& instance,vk::PhysicalDevice physicalDevice,const vk::DeviceCreateInfo& createInfo)
{
	_device=physicalDevice.createDevice(createInfo,nullptr,instance.funcs);
	funcs.vkGetDeviceProcAddr=PFN_vkGetDeviceProcAddr(_device.getProcAddr("vkGetDeviceProcAddr",instance.funcs));
	funcs.vkDestroyDevice=PFN_vkDestroyDevice(_device.getProcAddr("vkDestroyDevice",this->funcs));
}


template<typename T,VulkanDevice* device>
class VulkanUnique {
protected:
	T _value;
public:
	~VulkanUnique() {
		if(_value)  (*device)->destroy(_value,nullptr,device->funcs);
		_value=nullptr;
	}
};

static VulkanDevice vulkanDevice;
//static vk::DescriptorPool descriptorPool;

#include <vector>
vector<VulkanUnique<vk::DescriptorPool,&vulkanDevice>> poolList;
//vector<VulkanUnique<vk::Image,&vulkanDevice>> imageList;


int main(int,char**)
{
	// catch exceptions
	// (vulkan.hpp functions throws if they fail)
	try {

		// Vulkan library
		VulkanLibrary vulkanLib;
		vulkanLib.init();

		// Vulkan instance
		VulkanInstance vulkanInstance;
		vulkanInstance.init(
			vulkanLib,
			vk::InstanceCreateInfo{
				vk::InstanceCreateFlags(),  // flags
				&(const vk::ApplicationInfo&)vk::ApplicationInfo{
					"CADR tut02",            // application name
					VK_MAKE_VERSION(0,0,0),  // application version
					"CADR",                  // engine name
					VK_MAKE_VERSION(0,0,0),  // engine version
					VK_API_VERSION_1_0,      // api version
				},
				0,        // enabled layer count
				nullptr,  // enabled layer names
				0,        // enabled extension count
				nullptr,  // enabled extension names
			}
		);
#ifdef _WIN32
		HMODULE handle;
		GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		                   reinterpret_cast<LPCSTR>(vulkanInstance.funcs.vkDestroyInstance),&handle);
		WCHAR path[MAX_PATH];
		GetModuleFileNameW(handle,path,MAX_PATH);
		wcout<<"Library of vkDestroyInstance(): "<<path<<endl;
#else
		Dl_info dlInfo;
		dladdr(reinterpret_cast<void*>(vulkanInstance.funcs.vkDestroyInstance),&dlInfo);
		cout<<"Library of vkDestroyInstance(): "<<dlInfo.dli_fname<<endl;
#endif

		// print device list
		vector<vk::PhysicalDevice> deviceList=vulkanInstance->enumeratePhysicalDevices(vulkanInstance.funcs);
		cout<<"Physical devices:"<<endl;
		for(vk::PhysicalDevice pd:deviceList) {
			vk::PhysicalDeviceProperties p=pd.getProperties(vulkanInstance.funcs);
			cout<<"   "<<p.deviceName<<endl;

			VulkanDevice device(
				vulkanInstance,
				pd,
				vk::DeviceCreateInfo(
					vk::DeviceCreateFlags(),  // flags
					0,nullptr,  // no queues
					0,nullptr,  // no layers
					0,nullptr,  // no enabled extensions
					nullptr  // enabled features
				)
			);

#ifdef _WIN32
			GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			                   reinterpret_cast<LPCSTR>(device->getProcAddr("vkCreateShaderModule",device.funcs)),&handle);
			GetModuleFileNameW(handle,path,MAX_PATH);
			wcout<<"   Library of vkCreateShaderModule(): "<<path<<endl;
			GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			                   reinterpret_cast<LPCSTR>(device->getProcAddr("vkQueueSubmit",device.funcs)),&handle);
			GetModuleFileNameW(handle,path,MAX_PATH);
			wcout<<"   Library of vkQueueSubmit(): "<<path<<endl;
#else
			Dl_info dlInfo;
			auto vkCreateShaderModule=PFN_vkCreateShaderModule(device->getProcAddr("vkCreateShaderModule",device.funcs));
			dladdr(reinterpret_cast<void*>(vkCreateShaderModule),&dlInfo);
			cout<<"   Library of vkCreateShaderModule(): "<<dlInfo.dli_fname<<endl;
			auto vkQueueSubmit=PFN_vkQueueSubmit(device->getProcAddr("vkQueueSubmit",device.funcs));
			dladdr(reinterpret_cast<void*>(vkQueueSubmit),&dlInfo);
			cout<<"   Library of vkQueueSubmit(): "<<dlInfo.dli_fname<<endl;
#endif
		}

	// catch exceptions
	} catch(vk::Error &e) {
		cout<<"Failed because of Vulkan exception: "<<e.what()<<endl;
	} catch(exception &e) {
		cout<<"Failed because of exception: "<<e.what()<<endl;
	} catch(...) {
		cout<<"Failed because of unspecified exception."<<endl;
	}

	return 0;
}
