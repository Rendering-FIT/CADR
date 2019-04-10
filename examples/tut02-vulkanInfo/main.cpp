#include <vulkan/vulkan.hpp>
#include <iostream>

using namespace std;


int main(int,char**)
{
	// catch exceptions
	// (vulkan.hpp functions throws if they fail)
	try {

		// Vulkan version
		// (dynamic loading of vkEnumerateInstanceVersion() and some other functions must be used
		// as they might not be available on Vulkan 1.0. Some of them are available by extensions,
		// some are introduced by Vulkan 1.1.
		// vkEnumerateInstanceVersion() is available on Vulkan 1.1+ only. On Vulkan 1.0, it is nullptr.)
		struct VkFunc {
			PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion=PFN_vkEnumerateInstanceVersion(vk::Instance().getProcAddr("vkEnumerateInstanceVersion"));
			PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR;
			PFN_vkGetPhysicalDeviceDisplayPropertiesKHR vkGetPhysicalDeviceDisplayPropertiesKHR;
		} vkFunc;
		uint32_t version=(vkFunc.vkEnumerateInstanceVersion==nullptr)?VK_MAKE_VERSION(1,0,0):vk::enumerateInstanceVersion(vkFunc);
		cout<<"Vulkan info:"<<endl;
		cout<<"   Version:  "<<VK_VERSION_MAJOR(version)<<"."<<VK_VERSION_MINOR(version)<<"."<<VK_VERSION_PATCH(version)
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
		auto hasExtension=
			[](vector<vk::ExtensionProperties>& availableExtensions,const string& name) {
				return find_if(availableExtensions.begin(),availableExtensions.end(),
					[&name](vk::ExtensionProperties& ep){return strcmp(ep.extensionName,name.c_str())==0;})!=availableExtensions.end();
			};
		bool hasDeviceProperties2=hasExtension(availableExtensions,"VK_KHR_get_physical_device_properties2");
		bool hasKhrDisplay=hasExtension(availableExtensions,"VK_KHR_display");
		if(hasDeviceProperties2)
			enabledExtensions.emplace_back("VK_KHR_get_physical_device_properties2");
		if(hasKhrDisplay) {
			enabledExtensions.emplace_back("VK_KHR_display");
			enabledExtensions.emplace_back("VK_KHR_surface");  // dependency of VK_KHR_display
		}

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
		vkFunc.vkGetPhysicalDeviceProperties2KHR=PFN_vkGetPhysicalDeviceProperties2KHR(instance->getProcAddr("vkGetPhysicalDeviceProperties2KHR"));
		vector<vk::PhysicalDevice> deviceList=instance->enumeratePhysicalDevices();
		cout<<"Physical devices:"<<endl;
		for(vk::PhysicalDevice pd:deviceList) {
			vk::PhysicalDeviceProperties p=pd.getProperties();
			cout<<"   "<<p.deviceName<<endl;
			cout<<"      Vulkan version:  "<<VK_VERSION_MAJOR(p.apiVersion)<<"."<<VK_VERSION_MINOR(p.apiVersion)<<"."<<VK_VERSION_PATCH(p.apiVersion)<<endl;
			cout<<"      Driver version:  0x"<<hex<<p.driverVersion<<dec
			    <<" ("<<VK_VERSION_MAJOR(p.driverVersion)<<"."<<VK_VERSION_MINOR(p.driverVersion)<<"."<<VK_VERSION_PATCH(p.driverVersion)<<")"<<endl;
			cout<<"      VendorID:  0x"<<hex<<p.vendorID<<dec<<endl;
			cout<<"      DeviceID:  0x"<<hex<<p.deviceID<<dec<<endl;

			// get device properties
			vector<vk::ExtensionProperties> availableDeviceExtensions=pd.enumerateDeviceExtensionProperties();
			vk::PhysicalDeviceProperties2KHR p2;
#if VK_HEADER_VERSION<92
# pragma message("Old Vulkan header file detected. Some functionality will be disabled. Please, install new Vulkan SDK or use one from CADR/3rdParty folder.")
#endif
#if VK_HEADER_VERSION>=92
			vk::PhysicalDeviceDriverPropertiesKHR driverInfo;
			bool hasDriverInfo=hasExtension(availableDeviceExtensions,"VK_KHR_driver_properties");
			if(hasDriverInfo) {
				p2.pNext=&driverInfo;
				memset(driverInfo.driverName,'\0',sizeof(driverInfo.driverName));
				memset(driverInfo.driverInfo,'\0',sizeof(driverInfo.driverInfo));
			}
			vk::PhysicalDevicePCIBusInfoPropertiesEXT pciInfo;
			bool hasPciInfo=hasExtension(availableDeviceExtensions,"VK_EXT_pci_bus_info");
			if(hasPciInfo) {
				pciInfo.pNext=p2.pNext;
				p2.pNext=&pciInfo;
			}
#endif
			if(p2.pNext!=nullptr)
				pd.getProperties2KHR(&p2,vkFunc);

			// print driver info
#if VK_HEADER_VERSION>=92
			if(hasDriverInfo){
				cout<<"      Driver info:"<<endl;
				cout<<"         ID:    "<<vk::to_string(driverInfo.driverID)<<endl;
				cout<<"         Name:  "<<driverInfo.driverName<<endl;
				cout<<"         Info:  "<<driverInfo.driverInfo<<endl;
				cout<<"         Conformance version:  "<<driverInfo.conformanceVersion.major<<"."<<driverInfo.conformanceVersion.minor
				    <<"."<<driverInfo.conformanceVersion.subminor<<"."<<driverInfo.conformanceVersion.patch<<endl;
			}
			else
				cout<<"      Driver info:"<<endl
				    <<"         not available"<<endl;
#else
			cout<<"      Driver info:"<<endl
			    <<"         not available (support disabled during build time)"<<endl;
#endif
#if VK_HEADER_VERSION>=92
			if(hasPciInfo){
				cout<<"      PCI info:"<<endl;
				cout<<"         domain:  "<<pciInfo.pciDomain<<endl;
				cout<<"         bus:     "<<pciInfo.pciBus<<endl;
				cout<<"         device:  "<<pciInfo.pciDevice<<endl;
				cout<<"         function:  "<<pciInfo.pciFunction<<endl;
			}
			else
				cout<<"      PCI info:"<<endl
				    <<"         not available"<<endl;
#else
			cout<<"      PCI info:"<<endl
			    <<"         not available (support disabled during build time)"<<endl;
#endif

			// queues
			cout<<"      Queues:"<<endl;
			vector<vk::QueueFamilyProperties> queueFamilyList=pd.getQueueFamilyProperties();
			for(size_t i=0,c=queueFamilyList.size(); i<c; i++) {
				const vk::QueueFamilyProperties& qf=queueFamilyList[i];
				cout<<"         Family["<<i<<"]"<<endl;
				cout<<"            flags:  "<<vk::to_string(qf.queueFlags)<<endl;
				cout<<"            count:  "<<qf.queueCount<<endl;
			}

			// print device displays
			cout<<"      Displays:"<<endl;
			if(hasKhrDisplay) {
				try {
					vkFunc.vkGetPhysicalDeviceDisplayPropertiesKHR=PFN_vkGetPhysicalDeviceDisplayPropertiesKHR(instance->getProcAddr("vkGetPhysicalDeviceDisplayPropertiesKHR"));
					vector<vk::DisplayPropertiesKHR> dpList=pd.getDisplayPropertiesKHR(vkFunc);
					for(vk::DisplayPropertiesKHR& dp:dpList)
						cout<<"         "<<(dp.displayName?dp.displayName:"< no name >")<<", "
						    <<dp.physicalResolution.width<<"x"<<dp.physicalResolution.height<<endl;
					if(dpList.empty())
						cout<<"         < none >"<<endl;
				} catch(vk::Error& e) {
					cout<<"         VK_KHR_display extension raised exception: "<<e.what()<<endl;
				}
			} else
				cout<<"         < VK_KHR_display not supported >"<<endl;

			// extensions
			cout<<"      Extensions:"<<endl;
			for(vk::ExtensionProperties& e:availableDeviceExtensions)
				cout<<"         "<<e.extensionName<<" (version: "<<e.specVersion<<")"<<endl;

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
