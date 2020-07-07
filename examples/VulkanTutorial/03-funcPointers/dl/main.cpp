#include <iostream>
#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <codecvt>
# include <locale>
#else
# include <dlfcn.h>
#endif
#include <vulkan/vulkan.hpp>
#include "VulkanLibrary.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"

using namespace std;


// return name of library that contains the specified address (address of function, symbol, etc.)
string getLibraryOfAddr(void* addr)
{
#ifdef _WIN32

	// get module handle of belonging to address
	HMODULE handle;
	BOOL b=GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                          reinterpret_cast<LPCSTR>(addr),&handle);
	if(b==0||handle==nullptr)
		throw std::runtime_error("Function GetModuleHandleExA() failed.");

	// get module path
	WCHAR wpath[MAX_PATH];
	DWORD r=GetModuleFileNameW(handle,wpath,MAX_PATH);
	if(r==0||r==MAX_PATH)
		throw std::runtime_error("Function GetModuleFileNameW() failed.");

	// convert to utf-8
	char path[MAX_PATH];
	int l=WideCharToMultiByte(CP_UTF8,0,wpath,-1,path,MAX_PATH,NULL,NULL);
	if(l==0)
		throw std::runtime_error("Function WideCharToMultiByte() failed.");

	// return string
	return string(path,l);

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

		// Vulkan library and instance
		VulkanLibrary vulkanLib(VulkanLibrary::defaultName());
		VulkanInstance vulkanInstance(
			vulkanLib,               // Vulkan library
			"03-funcPointersDL",     // application name
			VK_MAKE_VERSION(0,0,0),  // application version
			nullptr,                 // engine name
			VK_MAKE_VERSION(0,0,0),  // engine version
			VK_API_VERSION_1_0,      // api version
			nullptr,                 // enabled layers
			nullptr                  // enabled extensions
		);

		// function pointer of vkCreateInstance()
		cout<<"Instance function pointers:"<<endl;
		cout<<"   vkCreateInstance()     points to: "<<getLibraryOfAddr(vulkanInstance.getProcAddr<void*>("vkCreateInstance"))<<endl;
		cout<<"   vkCreateShaderModule() points to: "<<getLibraryOfAddr(vulkanInstance.getProcAddr<void*>("vkCreateShaderModule"))<<endl;
		cout<<"   vkQueueSubmit()        points to: "<<getLibraryOfAddr(vulkanInstance.getProcAddr<void*>("vkQueueSubmit"))<<endl;

		// print device list
		vector<vk::PhysicalDevice> deviceList=vulkanInstance.enumeratePhysicalDevices();
		cout<<"Device function pointers:"<<endl;
		for(vk::PhysicalDevice pd:deviceList) {

			vk::PhysicalDeviceProperties p=vulkanInstance.getPhysicalDeviceProperties(pd);
			cout<<"   "<<p.deviceName<<endl;

			VulkanDevice device(
				vulkanInstance,  // instance
				pd,  // physicalDevice
				vk::DeviceCreateInfo(
					vk::DeviceCreateFlags(),  // flags
					1,  // at least one queue is mandatory
					&(const vk::DeviceQueueCreateInfo&)vk::DeviceQueueCreateInfo{  // pQueueCreateInfos
						vk::DeviceQueueCreateFlags(),  // flags
						0,  // queueFamilyIndex
						1,  // queueCount
						&(const float&)1.f,  // pQueuePriorities
					},
					0,nullptr,  // no layers
					0,nullptr,  // no enabled extensions
					nullptr  // enabled features
				)
			);

			// functions get from the device
			cout<<"      vkCreateShaderModule() points to: "<<getLibraryOfAddr(device.getProcAddr<void*>("vkCreateShaderModule"))<<endl;
			cout<<"      vkQueueSubmit()        points to: "<<getLibraryOfAddr(device.getProcAddr<void*>("vkQueueSubmit"))<<endl;

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
