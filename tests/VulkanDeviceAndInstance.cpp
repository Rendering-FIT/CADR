#include <CadR/VulkanInstance.h>
#include <CadR/VulkanDevice.h>

using namespace std;
using namespace CadR;


int main(int,char**)
{
	// VulkanInstance template methods
	VulkanInstance instance;
	instance.getProcAddr<PFN_vkCreateDevice>(string("a"));
	instance.getProcAddr<PFN_vkCreateDevice>(string("a").c_str());
	instance.enumeratePhysicalDevices();
	instance.enumeratePhysicalDevices(vector<vk::PhysicalDevice>::allocator_type());
	instance.getPhysicalDeviceProperties2(vk::PhysicalDevice());
	instance.enumerateDeviceExtensionProperties(vk::PhysicalDevice());
	instance.enumerateDeviceExtensionProperties(vk::PhysicalDevice(),string(""),vector<vk::ExtensionProperties>::allocator_type());
	instance.getPhysicalDeviceQueueFamilyProperties(vk::PhysicalDevice());
	instance.getPhysicalDeviceQueueFamilyProperties(vk::PhysicalDevice(),vector<vk::QueueFamilyProperties>::allocator_type());

	// VulkanInstance unique methods
	VulkanDevice device;
	vk::UniqueHandle<vk::Device,VulkanDevice> deviceHandle;
	instance.createDeviceUnique(vk::PhysicalDevice(),vk::DeviceCreateInfo(),device);
	instance.createDeviceUnique(vk::PhysicalDevice(),vk::DeviceCreateInfo(),nullptr,device);

	// VulkanDevice template methods
	device.getProcAddr<PFN_vkCreateDevice>(string("a"));
	device.getProcAddr<PFN_vkCreateDevice>(string("a").c_str());
	device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo());
	device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo(),vector<vk::DescriptorSet>::allocator_type());
	device.createGraphicsPipelines(vk::PipelineCache(),array{vk::GraphicsPipelineCreateInfo{}});
	device.createGraphicsPipelines(vk::PipelineCache(),array{vk::GraphicsPipelineCreateInfo{}},nullptr,vector<vk::Pipeline>::allocator_type());
	device.createComputePipelines(vk::PipelineCache(),array{vk::ComputePipelineCreateInfo{}});
	device.createComputePipelines(vk::PipelineCache(),array{vk::ComputePipelineCreateInfo{}},nullptr,vector<vk::Pipeline>::allocator_type());
	device.allocateCommandBuffers(vk::CommandBufferAllocateInfo());
	device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(),vector<vk::CommandBuffer>::allocator_type());
	device.cmdPushConstants<int>(vk::CommandBuffer(),vk::PipelineLayout(),vk::ShaderStageFlags(),0,array{int()});

	// VulkanDevice unique methods
	device.allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo());
	device.allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo(),allocator<vk::UniqueHandle<vk::DescriptorSet,VulkanDevice>>());
	device.createGraphicsPipelinesUnique(vk::PipelineCache(),array{vk::GraphicsPipelineCreateInfo{}});
	device.createGraphicsPipelinesUnique(vk::PipelineCache(),array{vk::GraphicsPipelineCreateInfo{}},nullptr,allocator<vk::UniqueHandle<vk::Pipeline,VulkanDevice>>());
	device.createComputePipelinesUnique(vk::PipelineCache(),array{vk::ComputePipelineCreateInfo{}});
	device.createComputePipelinesUnique(vk::PipelineCache(),array{vk::ComputePipelineCreateInfo{}},nullptr,allocator<vk::UniqueHandle<vk::Pipeline,VulkanDevice>>());
	device.allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo());
	device.allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(),allocator<vk::UniqueHandle<vk::CommandBuffer,VulkanDevice>>());

	// test return values
	vk::Result r1;
	vector<vk::Pipeline> r2;
	vk::Pipeline r3;
	vector<vk::UniqueHandle<vk::Pipeline,VulkanDevice>> r4;
	vk::UniqueHandle<vk::Pipeline,VulkanDevice> r5;
	r1=device.createGraphicsPipelines(vk::PipelineCache{},0,nullptr,nullptr,nullptr);
	r2=device.createGraphicsPipelines(vk::PipelineCache(),array{vk::GraphicsPipelineCreateInfo{}});
	r2=device.createGraphicsPipelines(vk::PipelineCache(),array{vk::GraphicsPipelineCreateInfo{}},nullptr,vector<vk::Pipeline>::allocator_type());
	r3=device.createGraphicsPipeline(vk::PipelineCache{},vk::GraphicsPipelineCreateInfo{});
	r4=device.createGraphicsPipelinesUnique(vk::PipelineCache(),array{vk::GraphicsPipelineCreateInfo{}});
	r4=device.createGraphicsPipelinesUnique(vk::PipelineCache(),array{vk::GraphicsPipelineCreateInfo{}},nullptr,allocator<vk::UniqueHandle<vk::Pipeline,VulkanDevice>>());
	r5=device.createGraphicsPipelineUnique(vk::PipelineCache(),vk::GraphicsPipelineCreateInfo{});
	if(r1!=vk::Result::eSuccess)  return 1;
	r1=device.createComputePipelines(vk::PipelineCache{},0,nullptr,nullptr,nullptr);
	r2=device.createComputePipelines(vk::PipelineCache(),array{vk::ComputePipelineCreateInfo{}});
	r2=device.createComputePipelines(vk::PipelineCache(),array{vk::ComputePipelineCreateInfo{}},nullptr,vector<vk::Pipeline>::allocator_type());
	r3=device.createComputePipeline(vk::PipelineCache{},vk::ComputePipelineCreateInfo{});
	r4=device.createComputePipelinesUnique(vk::PipelineCache(),array{vk::ComputePipelineCreateInfo{}});
	r4=device.createComputePipelinesUnique(vk::PipelineCache(),array{vk::ComputePipelineCreateInfo{}},nullptr,allocator<vk::UniqueHandle<vk::Pipeline,VulkanDevice>>());
	r5=device.createComputePipelineUnique(vk::PipelineCache(),vk::ComputePipelineCreateInfo{});
	if(r1!=vk::Result::eSuccess)  return 1;

	return 0;
}
