#include <vulkan/vulkan.hpp>
#include <iostream>
#include <stddef.h>
#ifdef _WIN32
#else
# include <dlfcn.h>
#endif

using namespace std;


// return name of library that contains the specified address (address of function, symbol, etc.)
string getLibraryOfAddr(void* addr)
{
#ifdef _WIN32
	HMODULE handle;
	GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                   reinterpret_cast<LPCSTR>(addr),&handle);
	WCHAR path[MAX_PATH];
	GetModuleFileNameW(handle,path,MAX_PATH);
	return string(path);
#else
	Dl_info dlInfo;
	dladdr(addr,&dlInfo);
	return string(dlInfo.dli_fname);
#endif
}


int main(int,char**)
{
	// catch exceptions
	// (vulkan.hpp functions throws if they fail)
	try {

		// Vulkan functions
		struct VkFuncs {
			PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
			PFN_vkCreateInstance vkCreateInstance;
			PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
			PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
			PFN_vkCreateDevice vkCreateDevice;
			PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
			PFN_vkCreateShaderModule vkCreateShaderModule;
			PFN_vkQueueSubmit vkQueueSubmit;
		} vkFuncs;

		// function pointers available without vk::Instance
#ifdef _WIN32
		vkFuncs.vkGetInstanceProcAddr=PFN_vkGetInstanceProcAddr(GetProcAddress(nullptr,"vkGetInstanceProcAddr"));
#else
		vkFuncs.vkGetInstanceProcAddr=PFN_vkGetInstanceProcAddr(dlsym(nullptr,"vkGetInstanceProcAddr"));
#endif
		vkFuncs.vkCreateInstance=PFN_vkCreateInstance(vk::Instance().getProcAddr("vkCreateInstance",vkFuncs));

		// Vulkan instance
		vk::UniqueInstance instance(
			vk::createInstance(
				vk::InstanceCreateInfo{
					vk::InstanceCreateFlags(),  // flags
					&(const vk::ApplicationInfo&)vk::ApplicationInfo{
						"tut02-funcPointers",    // application name
						VK_MAKE_VERSION(0,0,0),  // application version
						nullptr,                 // engine name
						VK_MAKE_VERSION(0,0,0),  // engine version
						VK_API_VERSION_1_0,      // api version
					},
					0,        // enabled layer count
					nullptr,  // enabled layer names
					0,        // enabled extension count
					nullptr,  // enabled extension names
				},
				nullptr,  // allocator
				vkFuncs));  // dispatch functions

		// function pointers get from instance
		vkFuncs.vkEnumeratePhysicalDevices=PFN_vkEnumeratePhysicalDevices(instance->getProcAddr("vkEnumeratePhysicalDevices",vkFuncs));
		vkFuncs.vkGetPhysicalDeviceProperties=PFN_vkGetPhysicalDeviceProperties(instance->getProcAddr("vkGetPhysicalDeviceProperties",vkFuncs));
		vkFuncs.vkCreateDevice=PFN_vkCreateDevice(instance->getProcAddr("vkCreateDevice",vkFuncs));
		vkFuncs.vkGetDeviceProcAddr=PFN_vkGetDeviceProcAddr(instance->getProcAddr("vkGetDeviceProcAddr",vkFuncs));
		cout<<"Library of vkCreateInstance(): "<<getLibraryOfAddr(reinterpret_cast<void*>(vkFuncs.vkCreateInstance))<<endl;

		// print device list
		vector<vk::PhysicalDevice> deviceList=instance->enumeratePhysicalDevices(vkFuncs);
		cout<<"Physical devices:"<<endl;
		for(vk::PhysicalDevice pd:deviceList) {

			vk::PhysicalDeviceProperties p=pd.getProperties(vkFuncs);
			cout<<"   "<<p.deviceName<<endl;

			vk::UniqueDevice device(  // Move constructor and physicalDevice.createDeviceUnique() does not work here because of bug
			                          // in vulkan.hpp until VK_HEADER_VERSION 73 (bug was fixed on 2018-03-05 in vulkan.hpp git).
			                          // Unfortunately, Ubuntu 18.04 carries still broken vulkan.hpp of VK_HEADER_VERSION 70.
				pd.createDevice(
					vk::DeviceCreateInfo(
						vk::DeviceCreateFlags(),  // flags
						0,nullptr,  // no queues
						0,nullptr,  // no layers
						0,nullptr,  // no enabled extensions
						nullptr  // enabled features
					),
					nullptr,  // allocator
					vkFuncs  // dispatch functions
				)
			);

			// functions get from device
			vkFuncs.vkCreateShaderModule=PFN_vkCreateShaderModule(device->getProcAddr("vkCreateShaderModule",vkFuncs));
			vkFuncs.vkQueueSubmit=PFN_vkQueueSubmit(device->getProcAddr("vkQueueSubmit",vkFuncs));
			cout<<"   Library of vkCreateShaderModule(): "<<getLibraryOfAddr(reinterpret_cast<void*>(vkFuncs.vkCreateShaderModule))<<endl;
			cout<<"   Library of vkQueueSubmit(): "<<getLibraryOfAddr(reinterpret_cast<void*>(vkFuncs.vkQueueSubmit))<<endl;

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
