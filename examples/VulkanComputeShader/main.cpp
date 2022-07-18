#include <vulkan/vulkan.hpp>
#include <iostream>

using namespace std;


// Vulkan instance
// (must be destructed as the last one)
static vk::UniqueInstance instance;

// Vulkan handles and objects
// (they need to be placed in particular (not arbitrary) order;
// this is because of their destruction order from bottom to up)
static vk::PhysicalDevice physicalDevice;
static uint32_t computeQueueFamily;
static vk::UniqueDevice device;
static vk::Queue computeQueue;
static vk::UniqueCommandPool commandPool;
static vk::UniqueCommandBuffer commandBuffer;
static vk::UniqueFence computeFinishedFence;

// shader code in SPIR-V binary
static const uint32_t computeShaderSpirv[]={
#include "shader.comp.spv"
};



/// main function of the application
int main(int,char**)
{
	// catch exceptions
	// (vulkan.hpp fuctions throw if they fail)
	try {

		// Vulkan instance
		instance=
			vk::createInstanceUnique(
				vk::InstanceCreateInfo{
					vk::InstanceCreateFlags(),  // flags
					&(const vk::ApplicationInfo&)vk::ApplicationInfo{
						"vulkanComputeShader",   // application name
						VK_MAKE_VERSION(0,0,0),  // application version
						nullptr,                 // engine name
						VK_MAKE_VERSION(0,0,0),  // engine version
						VK_API_VERSION_1_1,      // api version <-- need to be 1.1 for vkCmdDispatchBase
					},
					0,nullptr,  // no layers
					0,nullptr,  // no extensions
				});

		// find compatible devices
		// (the device must have a queue supporting graphics operations)
		vector<vk::PhysicalDevice> deviceList=instance->enumeratePhysicalDevices();
		vector<tuple<vk::PhysicalDevice,uint32_t>> compatibleDevices;
		for(vk::PhysicalDevice pd:deviceList) {

			// select queue for graphics rendering
			vector<vk::QueueFamilyProperties> queueFamilyList=pd.getQueueFamilyProperties();
			for(uint32_t i=0,c=uint32_t(queueFamilyList.size()); i<c; i++) {
				if(queueFamilyList[i].queueFlags & vk::QueueFlagBits::eCompute) {
					compatibleDevices.emplace_back(pd,i);
					break;
				}
			}
		}

		// print devices
		cout<<"Vulkan devices:"<<endl;
		for(vk::PhysicalDevice pd:deviceList)
			cout<<"   "<<pd.getProperties().deviceName<<endl;
		cout<<"Compatible devices:"<<endl;
		for(auto& t:compatibleDevices)
			cout<<"   "<<get<0>(t).getProperties().deviceName<<endl;

		// choose device
		if(compatibleDevices.empty())
			throw runtime_error("No compatible devices.");
		physicalDevice=get<0>(compatibleDevices.front());
		computeQueueFamily=get<1>(compatibleDevices.front());
		cout<<"Using device:\n"
		      "   "<<physicalDevice.getProperties().deviceName<<endl;

		// create device
		device=
			physicalDevice.createDeviceUnique(
				vk::DeviceCreateInfo{
					vk::DeviceCreateFlags(),  // flags
					1,                        // queueCreateInfoCount
					array{                    // pQueueCreateInfos
						vk::DeviceQueueCreateInfo{
							vk::DeviceQueueCreateFlags(),  // flags
							computeQueueFamily,  // queueFamilyIndex
							1,                    // queueCount
							&(const float&)1.f,   // pQueuePriorities
						},
					}.data(),
					0,nullptr,  // no layers
					1,array{"VK_KHR_shader_non_semantic_info"}.data(),  // number of enabled extensions, enabled extension names <-- VK_KHR_shader_non_semantic_info is required by debugPrintfEXT()
					nullptr,    // enabled features
				}
			);

		// get queues
		computeQueue=device->getQueue(computeQueueFamily,0);

		// shader
        vk::UniqueShaderModule computeShader =
            device->createShaderModuleUnique(
                    vk::ShaderModuleCreateInfo(
                        vk::ShaderModuleCreateFlags(),  // flags
                        sizeof(computeShaderSpirv),  // codeSize
                        computeShaderSpirv  // pCode
                    )
                );

		// pipeline
		vk::UniquePipelineLayout computePipelineLayout =
			device->createPipelineLayoutUnique(
				vk::PipelineLayoutCreateInfo{
					vk::PipelineLayoutCreateFlags(),  // flags
					0,       // setLayoutCount
					nullptr, // pSetLayouts
					0,       // pushConstantRangeCount
					nullptr  // pPushConstantRanges
				}
			);
		vk::UniquePipeline computePipeline =
			device->createComputePipelineUnique(
				nullptr,  // pipelineCache
				vk::ComputePipelineCreateInfo(  // createInfo
					vk::PipelineCreateFlags(),  // flags <-- use this for dispatchBase(0,0,0,...) 
					//vk::PipelineCreateFlagBits::eDispatchBase,  // flags <-- use this for dispatchBase(1,1,1,...)
					vk::PipelineShaderStageCreateInfo(  // stage
						vk::PipelineShaderStageCreateFlags(),  // flags
						vk::ShaderStageFlagBits::eCompute,  // stage
						computeShader.get(),  // module
						"main",  // pName
						nullptr  // pSpecializationInfo
					),
					computePipelineLayout.get(),  // layout
					nullptr,  // basePipelineHandle
					-1  // basePipelineIndex
				)
			).value;

		// command pool
		commandPool=
			device->createCommandPoolUnique(
				vk::CommandPoolCreateInfo(
					vk::CommandPoolCreateFlags(),  // flags
					computeQueueFamily  // queueFamilyIndex
				)
			);

		// allocate command buffer
		commandBuffer=std::move(
			device->allocateCommandBuffersUnique(
				vk::CommandBufferAllocateInfo(
					commandPool.get(),                 // commandPool
					vk::CommandBufferLevel::ePrimary,  // level
					1                                  // commandBufferCount
				)
			)[0]);

		// begin command buffer
		commandBuffer->begin(
			vk::CommandBufferBeginInfo(
				vk::CommandBufferUsageFlagBits::eOneTimeSubmit,  // flags
				nullptr  // pInheritanceInfo
			)
		);

		commandBuffer->bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline.get());
		//commandBuffer->dispatch(1,1,1); // <-- this works (debugPrintfEXT does its job)
		commandBuffer->dispatchBase(0,0,0,1,1,1);  // <-- this does not work (debugPrintfEXT does print nothing)
		//commandBuffer->dispatchBase(1,1,1,1,1,1);  // <-- this does not work as well

		// end command buffer
		commandBuffer->end();


		// fence
		computeFinishedFence=
			device->createFenceUnique(
				vk::FenceCreateInfo{
					vk::FenceCreateFlags()  // flags
				}
			);

		// submit work
		cout<<"Submiting work..."<<endl;
		computeQueue.submit(
			vk::SubmitInfo(  // submits
				0,nullptr,nullptr,       // waitSemaphoreCount, pWaitSemaphores, pWaitDstStageMask
				1,&commandBuffer.get(),  // commandBufferCount, pCommandBuffers
				0,nullptr                // signalSemaphoreCount, pSignalSemaphores
			),
			computeFinishedFence.get()  // fence
		);

		// wait for the work
		cout<<"Waiting for the work..."<<endl;
		vk::Result r=device->waitForFences(
			computeFinishedFence.get(),  // fences (vk::ArrayProxy)
			VK_TRUE,       // waitAll
			uint64_t(3e9)  // timeout (3s)
		);
		if(r==vk::Result::eTimeout)
			throw std::runtime_error("GPU timeout. Task is probably hanging.");

		cout<<"Done."<<endl;

	// catch exceptions
	} catch(vk::Error &e) {
		cout<<"Failed because of Vulkan exception: "<<e.what()<<endl;
	} catch(exception &e) {
		cout<<"Failed because of exception: "<<e.what()<<endl;
	} catch(...) {
		cout<<"Failed because of unspecified exception."<<endl;
	}

	// wait device idle, particularly, if there was an exception and device is busy
	// (device must be idle before destructors of buffers and other stuff are called)
	if(device)
		device->waitIdle();

	return 0;
}
