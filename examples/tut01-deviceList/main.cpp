#include <vulkan/vulkan.hpp>

int main(int,char**)
{
	// application info
	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName="CADR tut01";
	appInfo.applicationVersion=0;
	appInfo.pEngineName="CADR";
	appInfo.engineVersion=0;
	appInfo.apiVersion=VK_API_VERSION_1_0;

	// Vulkan instance
	vk::InstanceCreateInfo instanceCreateInfo;
	instanceCreateInfo.flags=vk::InstanceCreateFlags();
	instanceCreateInfo.pApplicationInfo=&appInfo;
	instanceCreateInfo.enabledLayerCount=0;
	instanceCreateInfo.ppEnabledLayerNames=nullptr;
	instanceCreateInfo.enabledExtensionCount=0;
	instanceCreateInfo.ppEnabledExtensionNames=nullptr;
	vk::Instance instance(vk::createInstance(instanceCreateInfo));

	return 0;
}
