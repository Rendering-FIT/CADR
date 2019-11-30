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


Renderer* Renderer::_instance = nullptr;



Renderer::Renderer(VulkanDevice* device,VulkanInstance* instance,vk::PhysicalDevice physicalDevice,uint32_t graphicsQueueFamily)
	: _device(device)
	, _graphicsQueueFamily(graphicsQueueFamily)
	, _emptyStorage(nullptr)
	, _indexAllocationManager(1024,0)  // set capacity to 1024, zero-sized null object (on index 0)
	, _primitiveSetAllocationManager(128,0)  // capacity, size of null object (on index 0)
	, _drawCommandAllocationManager(128,0)  // capacity, num null objects
{
	_attribStorages[AttribSizeList()].emplace_back(this,AttribSizeList()); // create empty AttribStorage for empty AttribSizeList (no attributes)
	_emptyStorage=&_attribStorages.begin()->second.front();
	_graphicsQueue=(*_device)->getQueue(_graphicsQueueFamily,0,*_device);
	_memoryProperties=physicalDevice.getMemoryProperties(*instance);

	// index buffer
	_indexBuffer=
		(*_device)->createBuffer(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				1024*sizeof(uint32_t),        // size
				vk::BufferUsageFlagBits::eIndexBuffer|vk::BufferUsageFlagBits::eTransferDst,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			),
			nullptr,  // allocator
			*_device  // dispatch
		);

	// index buffer memory
	_indexBufferMemory=allocateMemory(_indexBuffer,vk::MemoryPropertyFlagBits::eDeviceLocal);
	(*_device)->bindBufferMemory(
			_indexBuffer,  // buffer
			_indexBufferMemory,  // memory
			0,  // memoryOffset
			*_device  // dispatch
		);

	// primitiveSet buffer
	_primitiveSetBuffer=
		(*_device)->createBuffer(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				128*sizeof(PrimitiveSetGpuData),  // size
				vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			),
			nullptr,  // allocator
			*_device  // dispatch
		);

	// primitiveSet buffer memory
	_primitiveSetBufferMemory=allocateMemory(_primitiveSetBuffer,vk::MemoryPropertyFlagBits::eDeviceLocal);
	(*_device)->bindBufferMemory(
			_primitiveSetBuffer,  // buffer
			_primitiveSetBufferMemory,  // memory
			0,  // memoryOffset
			*_device  // dispatch
		);

	// drawCommand buffer
	_drawCommandBuffer=
		(*_device)->createBuffer(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				128*sizeof(DrawCommandGpuData),  // size
				vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			),
			nullptr,  // allocator
			*_device  // dispatch
		);

	// drawCommand buffer memory
	_drawCommandBufferMemory=allocateMemory(_drawCommandBuffer,vk::MemoryPropertyFlagBits::eDeviceLocal);
	(*_device)->bindBufferMemory(
			_drawCommandBuffer,  // buffer
			_drawCommandBufferMemory,  // memory
			0,  // memoryOffset
			*_device  // dispatch
		);

	// matrixListControl
	_matrixListControlBuffer=
		(*_device)->createBuffer(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				128*sizeof(8),  // size
				vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			),
			nullptr,  // allocator
			*_device  // dispatch
		);

	// matrixListControl buffer memory
	_matrixListControlBufferMemory=allocateMemory(_matrixListControlBuffer,vk::MemoryPropertyFlagBits::eDeviceLocal);
	(*_device)->bindBufferMemory(
			_matrixListControlBuffer,  // buffer
			_matrixListControlBufferMemory,  // memory
			0,  // memoryOffset
			*_device  // dispatch
		);

	// draw indirect buffer
	_drawIndirectBuffer=
		(*_device)->createBuffer(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				128*sizeof(vk::DrawIndexedIndirectCommand),  // size
				vk::BufferUsageFlagBits::eIndirectBuffer|vk::BufferUsageFlagBits::eStorageBuffer,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			),
			nullptr,  // allocator
			*_device  // dispatch
		);

	// draw indirect buffer memory
	_drawIndirectBufferMemory=allocateMemory(_drawIndirectBuffer,vk::MemoryPropertyFlagBits::eDeviceLocal);
	(*_device)->bindBufferMemory(
			_drawIndirectBuffer,  // buffer
			_drawIndirectBufferMemory,  // memory
			0,  // memoryOffset
			*_device  // dispatch
		);

	// stateSet buffer
	_stateSetBuffer=
		(*_device)->createBuffer(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				128*sizeof(4),  // size
				vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			),
			nullptr,  // allocator
			*_device  // dispatch
		);

	// stateSet buffer memory
	_stateSetBufferMemory=allocateMemory(_stateSetBuffer,vk::MemoryPropertyFlagBits::eDeviceLocal);
	(*_device)->bindBufferMemory(
			_stateSetBuffer,  // buffer
			_stateSetBufferMemory,  // memory
			0,  // memoryOffset
			*_device  // dispatch
		);

	// command pool used by StateSets
	_stateSetCommandPool=
		(*_device)->createCommandPool(
			vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlagBits::eResetCommandBuffer,  // flags
				_graphicsQueueFamily  // queueFamilyIndex
			),
			nullptr,  // allocator
			*_device  // dispatch
		);

	// descriptor set
	auto descriptorSetLayout=
		(*_device)->createDescriptorSetLayoutUnique(
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
			),
			nullptr,  // allocator
			*_device  // dispatch
		);
	auto descriptorPool=
		(*_device)->createDescriptorPoolUnique(
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
			),
			nullptr,  // allocator
			*_device  // dispatch
		);
	auto descriptorSet=
		(*_device)->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&descriptorSetLayout.get()  // pSetLayouts
			),
			*_device  // dispatch
		)[0];
	(*_device)->updateDescriptorSets(
		vk::WriteDescriptorSet(  // descriptorWrites
			descriptorSet,  // dstSet
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
		nullptr,  // descriptorCopies
		*_device  // dispatch
	);

	// processDrawCommands shader and pipeline stuff
	_processDrawCommandsShader=
		(*_device)->createShaderModule(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(processDrawCommandsShaderSpirv),  // codeSize
				processDrawCommandsShaderSpirv  // pCode
			),
			nullptr,  // allocator
			*_device  // dispatch
		);
	_pipelineCache=
		(*_device)->createPipelineCache(
			vk::PipelineCacheCreateInfo(
				vk::PipelineCacheCreateFlags(),  // flags
				0,       // initialDataSize
				nullptr  // pInitialData
			),
			nullptr,  // allocator
			*_device  // dispatch
		);
	auto pipelineLayout=
		(*_device)->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				1,       // setLayoutCount
				&descriptorSetLayout.get(), // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			},
			nullptr,  // allocator
			*_device  // dispatch
		);
	_processDrawCommandsPipeline=
		(*_device)->createComputePipeline(
			_pipelineCache,  // pipelineCache
			vk::ComputePipelineCreateInfo(  // createInfo
				vk::PipelineCreateFlags(),  // flags
				vk::PipelineShaderStageCreateInfo(  // stage
					vk::PipelineShaderStageCreateFlags(),  // flags
					vk::ShaderStageFlagBits::eCompute,  // stage
					_processDrawCommandsShader,  // module
					"main",  // pName
					nullptr  // pSpecializationInfo
				),
				pipelineLayout.get(),  // layout
				nullptr,  // basePipelineHandle
				-1  // basePipelineIndex
			),
			nullptr,  // allocator
			*_device  // dispatch
		);

	// transientCommandPool and uploadingCommandBuffer
	_transientCommandPool=
		(*_device)->createCommandPool(
			vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlagBits::eTransient|vk::CommandPoolCreateFlagBits::eResetCommandBuffer,  // flags
				_graphicsQueueFamily  // queueFamilyIndex
			),
			nullptr,  // allocator
			*_device  // dispatch
		);
	_uploadingCommandBuffer=
		(*_device)->allocateCommandBuffers(
			vk::CommandBufferAllocateInfo(
				_transientCommandPool,             // commandPool
				vk::CommandBufferLevel::ePrimary,  // level
				1                                  // commandBufferCount
			),
			*_device
		)[0];

	// start recording
	_uploadingCommandBuffer.begin(
		vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,  // flags
			nullptr  // pInheritanceInfo
		),
		*_device
	);
}


Renderer::~Renderer()
{
	assert((_emptyStorage==nullptr||_emptyStorage->allocationManager().numIDs()==1) && "Renderer::_emptyStorage is not empty. It is a programmer error to allocate anything there. You probably called Mesh::allocAttribs() without specifying AttribConfig.");
	assert(_drawCommandAllocationManager.numItems()==0 && "Renderer::_drawCommandAllocationManager still contains elements on Renderer destruction.");

	// destroy shaders, pipelines,...
	(*_device)->destroy(_processDrawCommandsShader,nullptr,*_device);
	(*_device)->destroy(_processDrawCommandsPipeline,nullptr,*_device);
	(*_device)->destroy(_pipelineCache,nullptr,*_device);

	// destroy StateSet CommandPool
	(*_device)->destroy(_stateSetCommandPool,nullptr,*_device);

	// clean up uploading operations
	_uploadingCommandBuffer.end(*_device);
	(*_device)->destroy(_transientCommandPool,nullptr,*_device);  // no need to destroy commandBuffers as destroying command pool frees all command buffers allocated from the pool
	purgeObjectsToDeleteAfterCopyOperation();

	// destroy buffers
	(*_device)->destroy(_indexBuffer,nullptr,*_device);
	(*_device)->freeMemory(_indexBufferMemory,nullptr,*_device);
	(*_device)->destroy(_primitiveSetBuffer,nullptr,*_device);
	(*_device)->freeMemory(_primitiveSetBufferMemory,nullptr,*_device);
	(*_device)->destroy(_drawCommandBuffer,nullptr,*_device);
	(*_device)->freeMemory(_drawCommandBufferMemory,nullptr,*_device);
	(*_device)->destroy(_matrixListControlBuffer,nullptr,*_device);
	(*_device)->freeMemory(_matrixListControlBufferMemory,nullptr,*_device);
	(*_device)->destroy(_drawIndirectBuffer,nullptr,*_device);
	(*_device)->freeMemory(_drawIndirectBufferMemory,nullptr,*_device);
	(*_device)->destroy(_stateSetBuffer,nullptr,*_device);
	(*_device)->freeMemory(_stateSetBufferMemory,nullptr,*_device);

	if(_instance==this)
		_instance=nullptr;
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
	vk::MemoryRequirements memoryRequirements=(*_device)->getBufferMemoryRequirements(buffer,*_device);
	for(uint32_t i=0; i<_memoryProperties.memoryTypeCount; i++)
		if(memoryRequirements.memoryTypeBits&(1<<i))
			if((_memoryProperties.memoryTypes[i].propertyFlags&requiredFlags)==requiredFlags)
				return
					(*_device)->allocateMemory(
						vk::MemoryAllocateInfo(
							memoryRequirements.size,  // allocationSize
							i                         // memoryTypeIndex
						),
						nullptr,*_device
					);
	throw std::runtime_error("No suitable memory type found for the buffer.");
}


void Renderer::scheduleCopyOperation(StagingBuffer& sb)
{
	_uploadingCommandBuffer.copyBuffer(sb._stgBuffer,sb._dstBuffer,1,
	                                   &(const vk::BufferCopy&)vk::BufferCopy(0,sb._dstOffset,sb._size),*_device);
	_objectsToDeleteAfterCopyOperation.emplace_back(sb._stgBuffer,sb._stgMemory);
	sb._stgBuffer=nullptr;
	sb._stgMemory=nullptr;
}


void Renderer::executeCopyOperations()
{
	// end recording
	_uploadingCommandBuffer.end(*_device);

	// submit command buffer
	auto fence=(*_device)->createFenceUnique(vk::FenceCreateInfo{vk::FenceCreateFlags()},nullptr,*_device);
	_graphicsQueue.submit(
		vk::SubmitInfo(  // submits (vk::ArrayProxy)
			0,nullptr,nullptr,           // waitSemaphoreCount,pWaitSemaphores,pWaitDstStageMask
			1,&_uploadingCommandBuffer,  // commandBufferCount,pCommandBuffers
			0,nullptr                    // signalSemaphoreCount,pSignalSemaphores
		),
		fence.get(),  // fence
		*_device      // dispatch
	);

	// wait for work to complete
	vk::Result r=(*_device)->waitForFences(
		fence.get(),    // fences (vk::ArrayProxy)
		VK_TRUE,        // waitAll
		uint64_t(3e9),  // timeout (3s)
		*_device        // dispatch
	);
	if(r==vk::Result::eTimeout)
		throw std::runtime_error("GPU timeout. Task is probably hanging.");

	purgeObjectsToDeleteAfterCopyOperation();

	// start new recoding
	_uploadingCommandBuffer.begin(
		vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,  // flags
			nullptr  // pInheritanceInfo
		),
		*_device  // dispatch
	);
}


void Renderer::purgeObjectsToDeleteAfterCopyOperation()
{
	for(auto item:_objectsToDeleteAfterCopyOperation) {
		(*_device)->destroy(std::get<0>(item),nullptr,*_device);
		(*_device)->freeMemory(std::get<1>(item),nullptr,*_device);
	}
	_objectsToDeleteAfterCopyOperation.clear();
}


StagingBuffer Renderer::createIndexStagingBuffer(Mesh& m)
{
	const ArrayAllocation<Mesh>& a=indexAllocation(m.indexDataID());
	return StagingBuffer(
			_indexBuffer,  // dstBuffer
			a.startIndex*sizeof(uint32_t),  // dstOffset
			a.numItems*sizeof(uint32_t),  // size
			this  // renderer
		);
}


StagingBuffer Renderer::createIndexStagingBuffer(Mesh& m,size_t firstIndex,size_t numIndices)
{
	const ArrayAllocation<Mesh>& a=indexAllocation(m.indexDataID());
	return StagingBuffer(
			_indexBuffer,  // dstBuffer
			(a.startIndex+firstIndex)*sizeof(uint32_t),  // dstOffset
			numIndices*sizeof(uint32_t),  // size
			this  // renderer
		);
}


void Renderer::uploadIndices(Mesh& m,std::vector<uint32_t>&& indexData,size_t dstIndex)
{
	// create StagingBuffer and submit it
	StagingBuffer sb(createIndexStagingBuffer(m,dstIndex,indexData.size()));
	memcpy(sb.data(),indexData.data(),indexData.size()*sizeof(uint32_t));
	sb.submit();
}


StagingBuffer Renderer::createPrimitiveSetStagingBuffer(Mesh& m)
{
	const ArrayAllocation<Mesh>& a=primitiveSetAllocation(m.primitiveSetDataID());
	return StagingBuffer(
			_primitiveSetBuffer,  // dstBuffer
			a.startIndex*sizeof(PrimitiveSetGpuData),  // dstOffset
			a.numItems*sizeof(PrimitiveSetGpuData),  // size
			this  // renderer
		);
}


StagingBuffer Renderer::createPrimitiveSetStagingBuffer(Mesh& m,size_t firstPrimitiveSet,size_t numPrimitiveSets)
{
	const ArrayAllocation<Mesh>& a=primitiveSetAllocation(m.primitiveSetDataID());
	return StagingBuffer(
			_primitiveSetBuffer,  // dstBuffer
			(a.startIndex+firstPrimitiveSet)*sizeof(PrimitiveSetGpuData),  // dstOffset
			numPrimitiveSets*sizeof(PrimitiveSetGpuData),  // size
			this  // renderer
		);
}


void Renderer::uploadPrimitiveSets(Mesh& m,std::vector<PrimitiveSetGpuData>&& primitiveSetData,size_t dstPrimitiveSet)
{
	// create StagingBuffer and submit it
	StagingBuffer sb(createPrimitiveSetStagingBuffer(m,dstPrimitiveSet,primitiveSetData.size()));
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
