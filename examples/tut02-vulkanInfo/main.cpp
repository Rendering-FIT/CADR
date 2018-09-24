#include <vulkan/vulkan.hpp>
#include <iostream>

using namespace std;


int main(int,char**)
{
	// catch exceptions
	// (vulkan.hpp fuctions throws if they fail)
	try {

		// vulkan version
		// (VulkanDispatchDynamic must be used as vkEnumerateInstanceVersion() is available since Vulkan 1.1 only.
		// On Vulkan 1.0, vkEnumerateInstanceVersion is nullptr.)
		struct VulkanDispatchDynamic {
			PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion=PFN_vkEnumerateInstanceVersion(vk::Instance().getProcAddr("vkEnumerateInstanceVersion"));;
		} d;
		uint32_t version=(d.vkEnumerateInstanceVersion==nullptr)?VK_MAKE_VERSION(1,0,0):vk::enumerateInstanceVersion(d);
		cout<<"Vulkan info:"<<endl;
		cout<<"   Version: "<<VK_VERSION_MAJOR(version)<<"."<<VK_VERSION_MINOR(version)<<"."<<VK_VERSION_PATCH(version)
		    <<" (header version: "<<VK_HEADER_VERSION<<")"<<endl;

		// extensions
		vector<vk::ExtensionProperties> availableExtensions=vk::enumerateInstanceExtensionProperties();
		cout<<"   Extensions:"<<endl;
		for(vk::ExtensionProperties& e:availableExtensions)
			cout<<"      "<<e.extensionName<<" (version: "<<e.specVersion<<")"<<endl;

		// layers
		vector<vk::LayerProperties> availableLayers=vk::enumerateInstanceLayerProperties();
		cout<<"   Layers:"<<endl;
		for(vk::LayerProperties& l:availableLayers)
			cout<<"      "<<l.layerName<<" (version: "<<l.specVersion<<")"<<endl;

		// extensions to enable
		vector<const char*> enabledExtensions;
		bool hasKhrDisplay=find_if(availableExtensions.begin(),availableExtensions.end(),
				[](vk::ExtensionProperties& ep){return strcmp(ep.extensionName,"VK_KHR_display")==0;})!=availableExtensions.end();
		if(hasKhrDisplay)  enabledExtensions.emplace_back("VK_KHR_display");

		// Vulkan instance
		vk::UniqueInstance instance(
			vk::createInstanceUnique(
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
					uint32_t(enabledExtensions.size()),  // enabled extension count
					enabledExtensions.data(),  // enabled extension names
				}));

		// print device list
		vector<vk::PhysicalDevice> deviceList=instance->enumeratePhysicalDevices();
		cout<<"Physical devices:"<<endl;
		for(vk::PhysicalDevice pd:deviceList) {
			vk::PhysicalDeviceProperties p=pd.getProperties();
			cout<<"   "<<p.deviceName<<endl;

			// print device displays
			cout<<"      Displays:"<<endl;
			if(hasKhrDisplay) {
				vector<vk::DisplayPropertiesKHR> dpList=pd.getDisplayPropertiesKHR();
				for(vk::DisplayPropertiesKHR& dp:dpList)
					cout<<"         "<<(dp.displayName?dp.displayName:"< no name >")<<", "
						 <<dp.physicalResolution.width<<"x"<<dp.physicalResolution.height<<endl;
				if(dpList.empty())
					cout<<"         < none >"<<endl;
			} else
				cout<<"         < VK_KHR_display not supported >"<<endl;
		}

	} catch(vk::Error &e) {
		cout<<"Failed because of Vulkan exception: "<<e.what()<<endl;
	} catch(exception &e) {
		cout<<"Failed because of exception: "<<e.what()<<endl;
	} catch(...) {
		cout<<"Failed because of unspecified exception."<<endl;
	}

	return 0;
}
