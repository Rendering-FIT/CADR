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
	vector<vk::PhysicalDevice>::allocator_type a1;
	instance.enumeratePhysicalDevices(a1);
	instance.getPhysicalDeviceProperties2(vk::PhysicalDevice());
	instance.enumerateDeviceExtensionProperties(vk::PhysicalDevice());
	vector<vk::ExtensionProperties>::allocator_type a2;
	instance.enumerateDeviceExtensionProperties(vk::PhysicalDevice(), string(""), a2);
	instance.getPhysicalDeviceQueueFamilyProperties(vk::PhysicalDevice());
	vector<vk::QueueFamilyProperties>::allocator_type a3;
	instance.getPhysicalDeviceQueueFamilyProperties(vk::PhysicalDevice(), a3);

	// VulkanInstance unique methods
	VulkanDevice device;
	vk::UniqueHandle<vk::Device,VulkanDevice> deviceHandle;
	instance.createDeviceUnique(vk::PhysicalDevice(),vk::DeviceCreateInfo(),device);
	instance.createDeviceUnique(vk::PhysicalDevice(),vk::DeviceCreateInfo(),nullptr,device);

	// VulkanDevice template methods
	device.getProcAddr<PFN_vkCreateDevice>(string("a"));
	device.getProcAddr<PFN_vkCreateDevice>(string("a").c_str());
	device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo());
	vector<vk::DescriptorSet>::allocator_type a4;
	device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo(), a4);
	device.createGraphicsPipelines(vk::PipelineCache(), array{vk::GraphicsPipelineCreateInfo{}});
	vector<vk::Pipeline>::allocator_type a5;
	device.createGraphicsPipelines(vk::PipelineCache(), array{vk::GraphicsPipelineCreateInfo{}}, nullptr, a5);
	device.createComputePipelines(vk::PipelineCache(), array{vk::ComputePipelineCreateInfo{}});
	vector<vk::Pipeline>::allocator_type a6;
	device.createComputePipelines(vk::PipelineCache(), array{vk::ComputePipelineCreateInfo{}}, nullptr, a6);
	device.allocateCommandBuffers(vk::CommandBufferAllocateInfo());
	vector<vk::CommandBuffer>::allocator_type a7;
	device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(), a7);
	device.cmdPushConstants<int>(vk::CommandBuffer(), vk::PipelineLayout(), vk::ShaderStageFlags(), 0, array{int()});

	// VulkanDevice unique methods
	device.allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo());
	allocator<vk::UniqueHandle<vk::DescriptorSet,VulkanDevice>> a8;
	device.allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo(), a8);
	device.createGraphicsPipelinesUnique(vk::PipelineCache(), array{vk::GraphicsPipelineCreateInfo{}});
	allocator<vk::UniqueHandle<vk::Pipeline,VulkanDevice>> a9;
	device.createGraphicsPipelinesUnique(vk::PipelineCache(), array{vk::GraphicsPipelineCreateInfo{}}, nullptr, a9);
	device.createComputePipelinesUnique(vk::PipelineCache(), array{vk::ComputePipelineCreateInfo{}});
	allocator<vk::UniqueHandle<vk::Pipeline,VulkanDevice>> a10;
	device.createComputePipelinesUnique(vk::PipelineCache(), array{vk::ComputePipelineCreateInfo{}}, nullptr, a10);
	device.allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo());
	allocator<vk::UniqueHandle<vk::CommandBuffer,VulkanDevice>> a11;
	device.allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(), a11);

	// test return values
	vk::Result r1;
	vector<vk::Pipeline> r2;
	vk::Pipeline r3;
	vector<vk::UniqueHandle<vk::Pipeline,VulkanDevice>> r4;
	vk::UniqueHandle<vk::Pipeline,VulkanDevice> r5;
	r1=device.createGraphicsPipelines(vk::PipelineCache{},0,nullptr,nullptr,nullptr);
	r2=device.createGraphicsPipelines(vk::PipelineCache(),array{vk::GraphicsPipelineCreateInfo{}});
	vector<vk::Pipeline>::allocator_type a12;
	r2=device.createGraphicsPipelines(vk::PipelineCache(), array{vk::GraphicsPipelineCreateInfo{}}, nullptr, a12);
	r3=device.createGraphicsPipeline(vk::PipelineCache{},vk::GraphicsPipelineCreateInfo{});
	r4=device.createGraphicsPipelinesUnique(vk::PipelineCache(), array{vk::GraphicsPipelineCreateInfo{}});
	allocator<vk::UniqueHandle<vk::Pipeline,VulkanDevice>> a13;
	r4=device.createGraphicsPipelinesUnique(vk::PipelineCache(), array{vk::GraphicsPipelineCreateInfo{}}, nullptr, a13);
	r5=device.createGraphicsPipelineUnique(vk::PipelineCache(),vk::GraphicsPipelineCreateInfo{});
	if(r1!=vk::Result::eSuccess)  return 1;
	r1=device.createComputePipelines(vk::PipelineCache{},0,nullptr,nullptr,nullptr);
	r2=device.createComputePipelines(vk::PipelineCache(),array{vk::ComputePipelineCreateInfo{}});
	vector<vk::Pipeline>::allocator_type a14;
	r2=device.createComputePipelines(vk::PipelineCache(), array{vk::ComputePipelineCreateInfo{}}, nullptr, a14);
	r3=device.createComputePipeline(vk::PipelineCache{},vk::ComputePipelineCreateInfo{});
	r4=device.createComputePipelinesUnique(vk::PipelineCache(), array{vk::ComputePipelineCreateInfo{}});
	allocator<vk::UniqueHandle<vk::Pipeline,VulkanDevice>> a15;
	r4=device.createComputePipelinesUnique(vk::PipelineCache(), array{vk::ComputePipelineCreateInfo{}}, nullptr, a15);
	r5=device.createComputePipelineUnique(vk::PipelineCache(),vk::ComputePipelineCreateInfo{});
	if(r1!=vk::Result::eSuccess)  return 1;

	return 0;
}
