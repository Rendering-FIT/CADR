#include <CadR/Renderer.h>
#include <CadR/AttribStorage.h>
#include <CadR/AllocationManagers.h>
#include <CadR/PrimitiveSet.h>
#include <CadR/StagingBuffer.h>
#include <CadR/VulkanDevice.h>
#include <CadR/VulkanInstance.h>

using namespace std;
using namespace CadR;

// shader code in SPIR-V binary
static const uint32_t processDrawCommandsShaderSpirv[]={
#include "shaders/processDrawCommands.comp.spv"
};


Renderer* Renderer::_defaultRenderer = nullptr;



Renderer::Renderer(bool makeDefault)
	: _device(nullptr)
	, _graphicsQueueFamily(0xffffffff)
	, _emptyStorage(nullptr)
	, _indexAllocationManager(1024,0)  // set capacity to 1024, zero-sized null object (on index 0)
	, _dataStorageAllocationManager(2048,0)  // set capacity to 2048, zero-sized null object (on index 0)
	, _primitiveSetAllocationManager(128,0)  // capacity, size of null object (on index 0)
	, _drawCommandAllocationManager(128,0)  // capacity, num null objects
{
	_attribStorages[AttribSizeList()].emplace_back(this,AttribSizeList()); // create empty AttribStorage for empty AttribSizeList (no attributes)
	_emptyStorage=&_attribStorages.begin()->second.front();
	if(makeDefault)
		Renderer::set(this);
}

Renderer::Renderer(VulkanDevice& device,VulkanInstance& instance,vk::PhysicalDevice physicalDevice,
                   uint32_t graphicsQueueFamily,bool makeDefault)
	: _device(nullptr)
	, _graphicsQueueFamily(graphicsQueueFamily)
	, _emptyStorage(nullptr)
	, _indexAllocationManager(1024,0)  // set capacity to 1024, zero-sized null object (on index 0)
	, _dataStorageAllocationManager(2048,0)  // set capacity to 2048, zero-sized null object (on index 0)
	, _primitiveSetAllocationManager(128,0)  // capacity, size of null object (on index 0)
	, _drawCommandAllocationManager(128,0)  // capacity, num null objects
{
	_attribStorages[AttribSizeList()].emplace_back(this,AttribSizeList()); // create empty AttribStorage for empty AttribSizeList (no attributes)
	_emptyStorage=&_attribStorages.begin()->second.front();
	init(device,instance,physicalDevice,graphicsQueueFamily,makeDefault);
}


Renderer::~Renderer()
{
	finalize();
	if(_defaultRenderer==this)
		_defaultRenderer=nullptr;
}


void Renderer::init(VulkanDevice& device,VulkanInstance& instance,vk::PhysicalDevice physicalDevice,
                    uint32_t graphicsQueueFamily,bool makeDefault)
{
	if(_device)
		finalize();

	if(makeDefault)
		Renderer::set(this);

	_device=&device;
	_graphicsQueueFamily=graphicsQueueFamily;
	_graphicsQueue=_device->getQueue(_graphicsQueueFamily,0);
	_memoryProperties=instance.getPhysicalDeviceMemoryProperties(physicalDevice);

	// nonCoherentAtomSize
	vk::DeviceSize nonCoherentAtomSize=instance.getPhysicalDeviceProperties(physicalDevice).limits.nonCoherentAtomSize;
	nonCoherentAtom_addition=nonCoherentAtomSize-1;
	nonCoherentAtom_mask=~nonCoherentAtom_addition;
	if((nonCoherentAtomSize&nonCoherentAtom_addition)!=0)  // is it power of two?
		throw std::runtime_error("Platform problem: nonCoherentAtomSize is not power of two.");

	// index buffer
	_indexBuffer=
		_device->createBuffer(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				1024*sizeof(uint32_t),        // size
				vk::BufferUsageFlagBits::eIndexBuffer|vk::BufferUsageFlagBits::eTransferDst,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			)
		);
	_indexBufferMemory=allocateMemory(_indexBuffer,vk::MemoryPropertyFlagBits::eDeviceLocal);
	_device->bindBufferMemory(
			_indexBuffer,  // buffer
			_indexBufferMemory,  // memory
			0  // memoryOffset
		);

	// data storage
	_dataStorageBuffer=
		_device->createBuffer(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				2048,                         // size
				vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			)
		);
	_dataStorageMemory=allocateMemory(_dataStorageBuffer,vk::MemoryPropertyFlagBits::eDeviceLocal);
	_device->bindBufferMemory(
			_dataStorageBuffer,  // buffer
			_dataStorageMemory,  // memory
			0  // memoryOffset
		);

	// primitiveSet buffer
	_primitiveSetBuffer=
		_device->createBuffer(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				128*sizeof(PrimitiveSetGpuData),  // size
				vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			)
		);
	_primitiveSetBufferMemory=allocateMemory(_primitiveSetBuffer,vk::MemoryPropertyFlagBits::eDeviceLocal);
	_device->bindBufferMemory(
			_primitiveSetBuffer,  // buffer
			_primitiveSetBufferMemory,  // memory
			0  // memoryOffset
		);

	// drawCommand buffer
	_drawCommandBuffer=
		_device->createBuffer(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				128*sizeof(DrawCommandGpuData),  // size
				vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			)
		);
	_drawCommandBufferMemory=allocateMemory(_drawCommandBuffer,vk::MemoryPropertyFlagBits::eDeviceLocal);
	_device->bindBufferMemory(
			_drawCommandBuffer,  // buffer
			_drawCommandBufferMemory,  // memory
			0  // memoryOffset
		);

	// matrixListControl
	_matrixListControlBuffer=
		_device->createBuffer(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				128*sizeof(8),  // size
				vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			)
		);
	_matrixListControlBufferMemory=allocateMemory(_matrixListControlBuffer,vk::MemoryPropertyFlagBits::eDeviceLocal);
	_device->bindBufferMemory(
			_matrixListControlBuffer,  // buffer
			_matrixListControlBufferMemory,  // memory
			0  // memoryOffset
		);

	// draw indirect buffer
	_drawIndirectBuffer=
		_device->createBuffer(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				128*sizeof(vk::DrawIndexedIndirectCommand),  // size
				vk::BufferUsageFlagBits::eIndirectBuffer|vk::BufferUsageFlagBits::eStorageBuffer,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			)
		);
	_drawIndirectBufferMemory=allocateMemory(_drawIndirectBuffer,vk::MemoryPropertyFlagBits::eDeviceLocal);
	_device->bindBufferMemory(
			_drawIndirectBuffer,  // buffer
			_drawIndirectBufferMemory,  // memory
			0  // memoryOffset
		);

	// stateSet buffer
	_stateSetBuffer=
		_device->createBuffer(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				128*sizeof(4),  // size
				vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			)
		);
	_stateSetBufferMemory=allocateMemory(_stateSetBuffer,vk::MemoryPropertyFlagBits::eDeviceLocal);
	_device->bindBufferMemory(
			_stateSetBuffer,  // buffer
			_stateSetBufferMemory,  // memory
			0  // memoryOffset
		);

	// stateSet staging buffer
	_stateSetStagingBuffer=
		_device->createBuffer(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				128*sizeof(4),  // size
				vk::BufferUsageFlagBits::eTransferSrc,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			)
		);
	_stateSetStagingMemory=allocateMemory(_stateSetStagingBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCached);
	_device->bindBufferMemory(
			_stateSetStagingBuffer,  // buffer
			_stateSetStagingMemory,  // memory
			0  // memoryOffset
		);
	_stateSetStagingData=reinterpret_cast<uint32_t*>(_device->mapMemory(_stateSetStagingMemory,0,VK_WHOLE_SIZE));

	// compute indirect buffer
	_computeIndirectBuffer=
		_device->createBuffer(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				3*sizeof(4),  // size
				vk::BufferUsageFlagBits::eIndirectBuffer,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			)
		);
	_computeIndirectBufferMemory=allocateMemory(_computeIndirectBuffer,vk::MemoryPropertyFlagBits::eHostVisible);
	_device->bindBufferMemory(
			_computeIndirectBuffer,  // buffer
			_computeIndirectBufferMemory,  // memory
			0  // memoryOffset
		);
	_computeIndirectBufferData=reinterpret_cast<uint32_t*>(_device->mapMemory(_computeIndirectBufferMemory,0,VK_WHOLE_SIZE));

	// command pool used by StateSets
	_stateSetCommandPool=
		_device->createCommandPool(
			vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlagBits::eResetCommandBuffer,  // flags
				_graphicsQueueFamily  // queueFamilyIndex
			)
		);

	// descriptor pool
	_drawCommandDescriptorPool=
		_device->createDescriptorPool(
			vk::DescriptorPoolCreateInfo(
				vk::DescriptorPoolCreateFlags(),  // flags
				1,  // maxSets
				1,  // poolSizeCount
				array<vk::DescriptorPoolSize,1>{  // pPoolSizes
					vk::DescriptorPoolSize(
						vk::DescriptorType::eStorageBuffer,  // type
						5  // descriptorCount
					),
				}.data()
			)
		);

	// descriptor set layout
	auto descriptorSetLayout=
		_device->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(  // createInfo
				vk::DescriptorSetLayoutCreateFlags(),  // flags
				5,  // bindingCount
				array<vk::DescriptorSetLayoutBinding,5>{  // pBindings
					vk::DescriptorSetLayoutBinding(
						0,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eCompute,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						1,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eCompute,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						2,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eCompute,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						3,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eCompute,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						4,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eCompute,  // stageFlags
						nullptr  // pImmutableSamplers
					)
				}.data()
			)
		);

	// allocate and update descriptor set
	_drawCommandDescriptorSet=
		_device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				_drawCommandDescriptorPool,  // descriptorPool
				1,  // descriptorSetCount
				&descriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	_device->updateDescriptorSets(
		vk::WriteDescriptorSet(  // descriptorWrites
			_drawCommandDescriptorSet,  // dstSet
			0,  // dstBinding
			0,  // dstArrayElement
			5,  // descriptorCount
			vk::DescriptorType::eStorageBuffer,  // descriptorType
			nullptr,  // pImageInfo
			array<vk::DescriptorBufferInfo,5>{  // pBufferInfo
				vk::DescriptorBufferInfo(
					_primitiveSetBuffer,  // buffer
					0,  // offset
					VK_WHOLE_SIZE  // range
				),
				vk::DescriptorBufferInfo(
					_drawCommandBuffer,  // buffer
					0,  // offset
					VK_WHOLE_SIZE  // range
				),
				vk::DescriptorBufferInfo(
					_matrixListControlBuffer,  // buffer
					0,  // offset
					VK_WHOLE_SIZE  // range
				),
				vk::DescriptorBufferInfo(
					_drawIndirectBuffer,  // buffer
					0,  // offset
					VK_WHOLE_SIZE  // range
				),
				vk::DescriptorBufferInfo(
					_stateSetBuffer,  // buffer
					0,  // offset
					VK_WHOLE_SIZE  // range
				),
			}.data(),
			nullptr  // pTexelBufferView
		),
		nullptr  // descriptorCopies
	);

	// processDrawCommands shader and pipeline stuff
	_drawCommandShader=
		_device->createShaderModule(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(processDrawCommandsShaderSpirv),  // codeSize
				processDrawCommandsShaderSpirv  // pCode
			)
		);
	_pipelineCache=
		_device->createPipelineCache(
			vk::PipelineCacheCreateInfo(
				vk::PipelineCacheCreateFlags(),  // flags
				0,       // initialDataSize
				nullptr  // pInitialData
			)
		);
	_drawCommandPipelineLayout=
		_device->createPipelineLayout(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				1,       // setLayoutCount
				&descriptorSetLayout.get(), // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			}
		);
	_drawCommandPipeline=
		_device->createComputePipeline(
			_pipelineCache,  // pipelineCache
			vk::ComputePipelineCreateInfo(  // createInfo
				vk::PipelineCreateFlags(),  // flags
				vk::PipelineShaderStageCreateInfo(  // stage
					vk::PipelineShaderStageCreateFlags(),  // flags
					vk::ShaderStageFlagBits::eCompute,  // stage
					_drawCommandShader,  // module
					"main",  // pName
					nullptr  // pSpecializationInfo
				),
				_drawCommandPipelineLayout,  // layout
				nullptr,  // basePipelineHandle
				-1  // basePipelineIndex
			)
		);

	// transientCommandPool and uploadingCommandBuffer
	_transientCommandPool=
		_device->createCommandPool(
			vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlagBits::eTransient|vk::CommandPoolCreateFlagBits::eResetCommandBuffer,  // flags
				_graphicsQueueFamily  // queueFamilyIndex
			)
		);
	_uploadingCommandBuffer=
		_device->allocateCommandBuffers(
			vk::CommandBufferAllocateInfo(
				_transientCommandPool,             // commandPool
				vk::CommandBufferLevel::ePrimary,  // level
				1                                  // commandBufferCount
			)
		)[0];

	// start recording
	_device->beginCommandBuffer(
		_uploadingCommandBuffer,  // commandBuffer
		vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,  // flags
			nullptr  // pInheritanceInfo
		)
	);
}


void Renderer::finalize()
{
	if(_device==nullptr)
		return;

	assert((_emptyStorage==nullptr||_emptyStorage->allocationManager().numIDs()==1) && "Renderer::_emptyStorage is not empty. It is a programmer error to allocate anything there. You probably called Geometry::allocAttribs() without specifying AttribSizeList.");
	assert(_drawCommandAllocationManager.numItems()==0 && "Renderer::_drawCommandAllocationManager still contains elements on Renderer destruction.");

	// clear attrib storages
	_attribStorages.clear();
	_attribStorages[AttribSizeList()].emplace_back(this,AttribSizeList()); // create empty AttribStorage for empty AttribSizeList (no attributes)
	_emptyStorage=&_attribStorages.begin()->second.front();

	// clear allocation managers
	_indexAllocationManager.clear();
	_dataStorageAllocationManager.clear();
	_primitiveSetAllocationManager.clear();
	_drawCommandAllocationManager.clear();

	// destroy shaders, pipelines,...
	_device->destroy(_drawCommandShader);
	_device->destroy(_drawCommandDescriptorPool);
	_device->destroy(_drawCommandPipelineLayout);
	_device->destroy(_drawCommandPipeline);
	_device->destroy(_pipelineCache);

	// destroy StateSet CommandPool
	_device->destroy(_stateSetCommandPool);

	// clean up uploading operations
	_device->endCommandBuffer(_uploadingCommandBuffer);
	_device->destroy(_transientCommandPool);  // no need to destroy commandBuffers as destroying command pool frees all command buffers allocated from the pool
	purgeObjectsToDeleteAfterCopyOperation();

	// destroy buffers
	_device->destroy(_indexBuffer);
	_device->freeMemory(_indexBufferMemory);
	_device->destroy(_dataStorageBuffer);
	_device->freeMemory(_dataStorageMemory);
	_device->destroy(_primitiveSetBuffer);
	_device->freeMemory(_primitiveSetBufferMemory);
	_device->destroy(_drawCommandBuffer);
	_device->freeMemory(_drawCommandBufferMemory);
	_device->destroy(_matrixListControlBuffer);
	_device->freeMemory(_matrixListControlBufferMemory);
	_device->destroy(_drawIndirectBuffer);
	_device->freeMemory(_drawIndirectBufferMemory);
	_device->destroy(_stateSetBuffer);
	_device->freeMemory(_stateSetBufferMemory);
	_device->destroy(_stateSetStagingBuffer);
	_device->freeMemory(_stateSetStagingMemory);  // no need to unmap memory as vkFreeMemory handles that
	_device->destroy(_computeIndirectBuffer);
	_device->freeMemory(_computeIndirectBufferMemory);  // no need to unmap memory as vkFreeMemory handles that

	_highestAllocatedSsId=-1;
	_releasedSsIds.clear();
	_stateSetStagingData=nullptr;
	_device=nullptr;
}


void Renderer::leakResources()
{
	// intentionally avoid all destructors
	_attribStorages.swap(*new decltype(_attribStorages));
	_device=nullptr;
}


void Renderer::recordDrawCommandProcessing(vk::CommandBuffer commandBuffer)
{
	// fill StateSet buffer with content
	_device->cmdCopyBuffer(
		commandBuffer,  // commandBuffer
		stateSetStagingBuffer(),  // srcBuffer
		stateSetBuffer(),  // dstBuffer
		vk::BufferCopy(
			0,  // srcOffset
			0,  // dstOffset
			numStateSetIds()*sizeof(uint32_t)  // size
		)  // pRegions
	);
	_device->cmdPipelineBarrier(
		commandBuffer,  // commandBuffer
		vk::PipelineStageFlagBits::eTransfer,  // srcStageMask
		vk::PipelineStageFlagBits::eComputeShader,  // dstStageMask
		vk::DependencyFlags(),  // dependencyFlags
		vk::MemoryBarrier(  // memoryBarriers
			vk::AccessFlagBits::eTransferWrite,  // srcAccessMask
			vk::AccessFlagBits::eShaderRead  // dstAccessMask
		),
		nullptr,  // bufferMemoryBarriers
		nullptr  // imageMemoryBarriers
	);

	// dispatch drawCommand compute pipeline
	_device->cmdBindPipeline(commandBuffer,vk::PipelineBindPoint::eCompute,drawCommandPipeline());
	_device->cmdBindDescriptorSets(
		commandBuffer,  // commandBuffer
		vk::PipelineBindPoint::eCompute,  // pipelineBindPoint
		drawCommandPipelineLayout(),  // layout
		0,  // firstSet
		drawCommandDescriptorSet(),  // descriptorSets
		nullptr  // dynamicOffsets
	);
	_device->cmdDispatchIndirect(commandBuffer,computeIndirectBuffer(),0);
	_device->cmdPipelineBarrier(
		commandBuffer,  // commandBuffer
		vk::PipelineStageFlagBits::eComputeShader,  // srcStageMask
		vk::PipelineStageFlagBits::eDrawIndirect,  // dstStageMask
		vk::DependencyFlags(),  // dependencyFlags
		vk::MemoryBarrier(  // memoryBarriers
			vk::AccessFlagBits::eShaderWrite,  // srcAccessMask
			vk::AccessFlagBits::eIndirectCommandRead  // dstAccessMask
		),
		nullptr,  // bufferMemoryBarriers
		nullptr  // imageMemoryBarriers
	);
}


void Renderer::recordSceneRendering(vk::CommandBuffer commandBuffer,StateSet* stateSetRoot,vk::RenderPass renderPass,
                                    vk::Framebuffer framebuffer,const vk::Rect2D& renderArea,
                                    uint32_t clearValueCount,const vk::ClearValue* clearValues)
{
	// start render pass
	_device->cmdBeginRenderPass(
		commandBuffer,  // commandBuffer
		vk::RenderPassBeginInfo(
			renderPass,   // renderPass
			framebuffer,  // framebuffer
			renderArea,   // renderArea
			clearValueCount,  // clearValueCount
			clearValues   // pClearValues
		),
		vk::SubpassContents::eInline  // contents
	);
	_device->cmdBindIndexBuffer(
		commandBuffer,  // commandBuffer
		indexBuffer(),  // buffer
		0,  // offset
		vk::IndexType::eUint32  // indexType
	);

	// execute all StateSets
	vk::DeviceSize indirectBufferOffset=0;
	stateSetRoot->recordToCommandBuffer(commandBuffer,indirectBufferOffset);
	size_t numDrawCommands=indirectBufferOffset/sizeof(vk::DrawIndexedIndirectCommand);
	_device->flushMappedMemoryRanges(
		vk::MappedMemoryRange(
			stateSetStagingMemory(),  // memory
			0,  // offset
			(numStateSetIds()*sizeof(uint32_t)+nonCoherentAtom_addition)&nonCoherentAtom_mask  // size - rounded up to next nonCoherentAtomSize
		)
	);
	_device->cmdEndRenderPass(commandBuffer);

	// update compute indirect data
	computeIndirectBufferData()[0]=uint32_t(numDrawCommands);
	computeIndirectBufferData()[1]=1;
	computeIndirectBufferData()[2]=1;
	_device->flushMappedMemoryRanges(
		vk::MappedMemoryRange(
			computeIndirectBufferMemory(),  // memory
			0,  // offset
			VK_WHOLE_SIZE  // size
		)
	);
}


AttribStorage* Renderer::getOrCreateAttribStorage(const AttribSizeList& attribSizeList)
{
	std::list<AttribStorage>& attribStorageList=_attribStorages[attribSizeList];
	if(attribStorageList.empty())
		attribStorageList.emplace_front(AttribStorage(this,attribSizeList));
	return &attribStorageList.front();
}


vk::DeviceMemory Renderer::allocateMemory(vk::Buffer buffer,vk::MemoryPropertyFlags requiredFlags)
{
	vk::MemoryRequirements memoryRequirements=_device->getBufferMemoryRequirements(buffer);
	for(uint32_t i=0; i<_memoryProperties.memoryTypeCount; i++)
		if(memoryRequirements.memoryTypeBits&(1<<i))
			if((_memoryProperties.memoryTypes[i].propertyFlags&requiredFlags)==requiredFlags)
				return
					_device->allocateMemory(
						vk::MemoryAllocateInfo(
							memoryRequirements.size,  // allocationSize
							i                         // memoryTypeIndex
						)
					);
	throw std::runtime_error("No suitable memory type found for the buffer.");
}


void Renderer::scheduleCopyOperation(StagingBuffer& sb)
{
	_device->cmdCopyBuffer(_uploadingCommandBuffer,sb._stgBuffer,sb._dstBuffer,
	                       vk::BufferCopy(0,sb._dstOffset,sb._size));
	_objectsToDeleteAfterCopyOperation.emplace_back(sb._stgBuffer,sb._stgMemory);
	sb._stgBuffer=nullptr;
	sb._stgMemory=nullptr;
}


void Renderer::executeCopyOperations()
{
	// end recording
	_device->endCommandBuffer(_uploadingCommandBuffer);

	// submit command buffer
	auto fence=_device->createFenceUnique(vk::FenceCreateInfo{vk::FenceCreateFlags()});
	_device->queueSubmit(
		_graphicsQueue,  // queue
		vk::SubmitInfo(  // submits (vk::ArrayProxy)
			0,nullptr,nullptr,           // waitSemaphoreCount,pWaitSemaphores,pWaitDstStageMask
			1,&_uploadingCommandBuffer,  // commandBufferCount,pCommandBuffers
			0,nullptr                    // signalSemaphoreCount,pSignalSemaphores
		),
		fence.get()  // fence
	);

	// wait for work to complete
	vk::Result r=_device->waitForFences(
		fence.get(),   // fences (vk::ArrayProxy)
		VK_TRUE,       // waitAll
		uint64_t(3e9)  // timeout (3s)
	);
	if(r==vk::Result::eTimeout)
		throw std::runtime_error("GPU timeout. Task is probably hanging.");

	purgeObjectsToDeleteAfterCopyOperation();

	// start new recoding
	_device->beginCommandBuffer(
		_uploadingCommandBuffer,  // commandBuffer
		vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,  // flags
			nullptr  // pInheritanceInfo
		)
	);
}


void Renderer::purgeObjectsToDeleteAfterCopyOperation()
{
	for(auto item:_objectsToDeleteAfterCopyOperation) {
		_device->destroy(std::get<0>(item));
		_device->freeMemory(std::get<1>(item));
	}
	_objectsToDeleteAfterCopyOperation.clear();
}


StagingBuffer Renderer::createIndexStagingBuffer(Geometry& g)
{
	const ArrayAllocation<Geometry>& a=indexAllocation(g.indexDataID());
	return StagingBuffer(
			_indexBuffer,  // dstBuffer
			a.startIndex*sizeof(uint32_t),  // dstOffset
			a.numItems*sizeof(uint32_t),  // size
			this  // renderer
		);
}


StagingBuffer Renderer::createIndexStagingBuffer(Geometry& g,size_t firstIndex,size_t numIndices)
{
	const ArrayAllocation<Geometry>& a=indexAllocation(g.indexDataID());
	assert(a.numItems>=numIndices && "Renderer::createIndexStagingBuffer(): Parameter numIndices is bigger than allocated space in Geometry.");
	return StagingBuffer(
			_indexBuffer,  // dstBuffer
			(a.startIndex+firstIndex)*sizeof(uint32_t),  // dstOffset
			numIndices*sizeof(uint32_t),  // size
			this  // renderer
		);
}


void Renderer::uploadIndices(Geometry& g,std::vector<uint32_t>&& indexData,size_t dstIndex)
{
	if(indexData.empty()) return;

	// create StagingBuffer and submit it
	StagingBuffer sb(createIndexStagingBuffer(g,dstIndex,indexData.size()));
	memcpy(sb.data(),indexData.data(),indexData.size()*sizeof(uint32_t));
	sb.submit();
}


StagingBuffer Renderer::createDataStorageStagingBuffer(uint32_t id)
{
	const ArrayAllocation<uint32_t>& a=dataStorageAllocation(id);
	return StagingBuffer(
			_dataStorageBuffer,  // dstBuffer
			a.startIndex,  // dstOffset
			a.numItems,  // size
			this  // renderer
		);
}


StagingBuffer Renderer::createDataStorageStagingBuffer(uint32_t id,size_t offset,size_t size)
{
	const ArrayAllocation<uint32_t>& a=dataStorageAllocation(id);
	assert(a.numItems>=size && "Renderer::createDataStorageStagingBuffer(): Parameter size is bigger than allocated space.");
	return StagingBuffer(
			_dataStorageBuffer,  // dstBuffer
			a.startIndex+offset,  // dstOffset
			size,  // size
			this  // renderer
		);
}


void Renderer::uploadDataStorage(uint32_t id,std::vector<uint8_t>&& data,size_t dstIndex)
{
	if(data.empty()) return;

	// create StagingBuffer and submit it
	StagingBuffer sb(createDataStorageStagingBuffer(id,dstIndex,data.size()));
	memcpy(sb.data(),data.data(),data.size());
	sb.submit();
}


void Renderer::uploadDataStorage(uint32_t id,const void* data,size_t size,size_t dstIndex)
{
	if(size==0) return;

	// create StagingBuffer and submit it
	StagingBuffer sb(createDataStorageStagingBuffer(id,dstIndex,size));
	memcpy(sb.data(),data,size);
	sb.submit();
}


StagingBuffer Renderer::createPrimitiveSetStagingBuffer(Geometry& g)
{
	const ArrayAllocation<Geometry>& a=primitiveSetAllocation(g.primitiveSetDataID());
	return StagingBuffer(
			_primitiveSetBuffer,  // dstBuffer
			a.startIndex*sizeof(PrimitiveSetGpuData),  // dstOffset
			a.numItems*sizeof(PrimitiveSetGpuData),  // size
			this  // renderer
		);
}


StagingBuffer Renderer::createPrimitiveSetStagingBuffer(Geometry& g,size_t firstPrimitiveSet,size_t numPrimitiveSets)
{
	const ArrayAllocation<Geometry>& a=primitiveSetAllocation(g.primitiveSetDataID());
	assert(a.numItems>=numPrimitiveSets && "Renderer::createPrimitiveSetStagingBuffer(): Parameter numPrimitiveSets is bigger than allocated space in Geometry.");
	return StagingBuffer(
			_primitiveSetBuffer,  // dstBuffer
			(a.startIndex+firstPrimitiveSet)*sizeof(PrimitiveSetGpuData),  // dstOffset
			numPrimitiveSets*sizeof(PrimitiveSetGpuData),  // size
			this  // renderer
		);
}


void Renderer::uploadPrimitiveSets(Geometry& g,std::vector<PrimitiveSetGpuData>&& primitiveSetData,size_t dstPrimitiveSetIndex)
{
	if(primitiveSetData.empty()) return;

	// create StagingBuffer and submit it
	StagingBuffer sb(createPrimitiveSetStagingBuffer(g,dstPrimitiveSetIndex,primitiveSetData.size()));
	memcpy(sb.data(),primitiveSetData.data(),primitiveSetData.size()*sizeof(PrimitiveSetGpuData));
	sb.submit();
}


StagingBuffer Renderer::createDrawCommandStagingBuffer(DrawCommand& dc)
{
	assert(dc.index()!=ItemAllocationManager::invalidID && "Can not create StagingBuffer for DrawCommand with invalid ID.");
	return StagingBuffer(
			_drawCommandBuffer,  // dstBuffer
			dc.index()*sizeof(DrawCommandGpuData),  // dstOffset
			sizeof(DrawCommandGpuData),  // size
			this  // renderer
		);
}


void Renderer::uploadDrawCommand(DrawCommand& dc,const DrawCommandGpuData& drawCommandData)
{
	// create StagingBuffer and submit it
	StagingBuffer sb(createDrawCommandStagingBuffer(dc));
	memcpy(sb.data(),&drawCommandData,sizeof(DrawCommandGpuData));
	sb.submit();
}


uint32_t Renderer::allocateStateSetId()
{
	uint32_t r;
	if(_releasedSsIds.empty()) {
		_highestAllocatedSsId++;
		r=_highestAllocatedSsId;
	} else {
		r=_releasedSsIds.back();
		_releasedSsIds.pop_back();
	}
	return r;
}


void Renderer::releaseStateSetId(uint32_t id)
{
	if(id!=_highestAllocatedSsId)

		// just put the offset into the queue of released offsets
		_releasedSsIds.push_back(id);

	else {

		// remove all highest released offsets
		while(true) {
			_highestAllocatedSsId--;
			auto it=std::max_element(_releasedSsIds.begin(),_releasedSsIds.end());
			if(it==_releasedSsIds.end() || *it!=_highestAllocatedSsId)
				break;
			*it=_releasedSsIds.back();
			_releasedSsIds.pop_back();
		}

	}
}
