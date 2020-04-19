#include <vulkan/vulkan.hpp>
#include <iostream>

using namespace std;


// main function of the application
int main(int,char**)
{
	// catch exceptions
	// (vulkan.hpp fuctions throw if they fail)
	try {

		// Vulkan version
		auto vkEnumerateInstanceVersion=reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
				vkGetInstanceProcAddr(nullptr,"vkEnumerateInstanceVersion"));
		if(vkEnumerateInstanceVersion) {
			uint32_t version;
			vkEnumerateInstanceVersion(&version);
			cout<<"Vulkan instance version: "<<VK_VERSION_MAJOR(version)<<"."<<VK_VERSION_MINOR(version)
			    <<"."<<VK_VERSION_PATCH(version)<<endl;
		} else
			cout<<"Vulkan version: 1.0"<<endl;

		// Vulkan instance
		vk::UniqueInstance instance(
			vk::createInstanceUnique(
				vk::InstanceCreateInfo{
					vk::InstanceCreateFlags(),  // flags
					&(const vk::ApplicationInfo&)vk::ApplicationInfo{
						"04-deviceInfo",         // application name
						VK_MAKE_VERSION(0,0,0),  // application version
						nullptr,                 // engine name
						VK_MAKE_VERSION(0,0,0),  // engine version
						VK_API_VERSION_1_0,      // api version
					},
					0,nullptr,  // no layers
					0,nullptr,  // no extensions
				}));

		// print device info
		vector<vk::PhysicalDevice> deviceList=instance->enumeratePhysicalDevices();
		cout<<"Physical devices:"<<endl;
		for(vk::PhysicalDevice pd:deviceList) {

			// device properties
			vk::PhysicalDeviceProperties p=pd.getProperties();
			cout<<"   "<<p.deviceName<<endl;

			// device Vulkan version
			cout<<"      Vulkan version: "<<VK_VERSION_MAJOR(p.apiVersion)<<"."<<VK_VERSION_MINOR(p.apiVersion)
			    <<"."<<VK_VERSION_PATCH(p.apiVersion)<<endl;

			// device limits
			cout<<"      MaxTextureSize: "<<p.limits.maxImageDimension2D<<endl;

			// device features
			vk::PhysicalDeviceFeatures f=pd.getFeatures();
			cout<<"      Geometry shader: ";
			if(f.geometryShader)
				cout<<"supported"<<endl;
			else
				cout<<"not supported"<<endl;

			// memory properties
			vk::PhysicalDeviceMemoryProperties m=pd.getMemoryProperties();
			cout<<"      Memory heaps:"<<endl;
			for(uint32_t i=0,c=m.memoryHeapCount; i<c; i++)
				cout<<"         "<<i<<": "<<m.memoryHeaps[i].size/1024/1024<<"MiB"<<endl;

			// queue family properties
			vector<vk::QueueFamilyProperties> queueFamilyList=pd.getQueueFamilyProperties();
			cout<<"      Queues:"<<endl;
			for(uint32_t i=0,c=uint32_t(queueFamilyList.size()); i<c; i++) {
				cout<<"         "<<i<<": ";
				if(queueFamilyList[i].queueFlags & vk::QueueFlagBits::eGraphics)
					cout<<"g";
				if(queueFamilyList[i].queueFlags & vk::QueueFlagBits::eCompute)
					cout<<"c";
				if(queueFamilyList[i].queueFlags & vk::QueueFlagBits::eTransfer)
					cout<<"t";
				cout<<endl;
			}

			// color attachment R8G8B8A8Unorm format support
			vk::FormatProperties fp = pd.getFormatProperties(vk::Format::eR8G8B8A8Unorm);
			cout<<"      R8G8B8A8Unorm format support:"<<endl;
			cout<<"         Images with linear tiling: "<<
				string(fp.linearTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment ? "yes" : "no")<<endl;
			cout<<"         Images with optimal tiling: "<<
				string(fp.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment ? "yes" : "no")<<endl;
			cout<<"         Buffers: "<<
				string(fp.bufferFeatures & vk::FormatFeatureFlagBits::eColorAttachment ? "yes" : "no")<<endl;

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
