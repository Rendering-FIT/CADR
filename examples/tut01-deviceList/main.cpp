#include <vulkan/vulkan.hpp>
#include <iostream>

using namespace std;


int main(int,char**)
{
	// catch exceptions
	// (vulkan.hpp functions throws if they fail)
	try {

		// Vulkan instance
		vk::UniqueInstance instance(
			vk::createInstanceUnique(
				vk::InstanceCreateInfo{
					vk::InstanceCreateFlags(),  // flags
					&(const vk::ApplicationInfo&)vk::ApplicationInfo{
						"CADR tut01",            // application name
						VK_MAKE_VERSION(0,0,0),  // application version
						"CADR",                  // engine name
						VK_MAKE_VERSION(0,0,0),  // engine version
						VK_API_VERSION_1_0,      // api version
					},
					0,        // enabled layer count
					nullptr,  // enabled layer names
					0,        // enabled extension count
					nullptr,  // enabled extension names
				}));

		// print device list
		vector<vk::PhysicalDevice> deviceList=instance->enumeratePhysicalDevices();
		cout<<"Physical devices:"<<endl;
		for(vk::PhysicalDevice pd:deviceList) {
			vk::PhysicalDeviceProperties p=pd.getProperties();
			cout<<"   "<<p.deviceName<<endl;
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
