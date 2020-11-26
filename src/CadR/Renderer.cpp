#include <CadR/Renderer.h>
#include <CadR/AllocationManagers.h>
#include <CadR/PrimitiveSet.h>
#include <CadR/StagingBuffer.h>
#include <CadR/VertexStorage.h>
#include <CadR/VulkanDevice.h>
#include <CadR/VulkanInstance.h>

using namespace std;
using namespace CadR;

// shader code in SPIR-V binary
static const uint32_t processDrawablesShaderSpirv[]={
#include "shaders/processDrawables.comp.spv"
};


Renderer* Renderer::_defaultRenderer = nullptr;



Renderer::Renderer(bool makeDefault)
	: _device(nullptr)
	, _graphicsQueueFamily(0xffffffff)
	, _emptyStorage(nullptr)
	, _indexAllocationManager(1024,0)  // set capacity to 1024, zero-sized null object (on index 0)
	, _dataStorageAllocationManager(4096,0)  // set capacity to 4096, zero-sized null object (on index 0)
	, _primitiveSetAllocationManager(128,0)  // capacity, size of null object (on index 0)
{
	_vertexStorages[AttribSizeList()].emplace_back(this,AttribSizeList()); // create empty VertexStorage for empty AttribSizeList (no attributes)
	_emptyStorage=&_vertexStorages.begin()->second.front();
	if(makeDefault)
		Renderer::set(this);
}

Renderer::Renderer(VulkanDevice& device,VulkanInstance& instance,vk::PhysicalDevice physicalDevice,
                   uint32_t graphicsQueueFamily,bool makeDefault)
	: _device(nullptr)
	, _graphicsQueueFamily(graphicsQueueFamily)
	, _emptyStorage(nullptr)
	, _indexAllocationManager(1024,0)  // set capacity to 1024, zero-sized null object (on index 0)
	, _dataStorageAllocationManager(4096,0)  // set capacity to 4096, zero-sized null object (on index 0)
	, _primitiveSetAllocationManager(128,0)  // capacity, size of null object (on index 0)
{
	_vertexStorages[AttribSizeList()].emplace_back(this,AttribSizeList()); // create empty VertexStorage for empty AttribSizeList (no attributes)
	_emptyStorage=&_vertexStorages.begin()->second.front();
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
				4096,                         // size
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

	// create general purpose fence
	_fence=
		_device->createFence(vk::FenceCreateInfo{vk::FenceCreateFlags()});

	// descriptor pool
	_drawableDescriptorPool=
		_device->createDescriptorPool(
			vk::DescriptorPoolCreateInfo(
				vk::DescriptorPoolCreateFlags(),  // flags
				1,  // maxSets
				1,  // poolSizeCount
				array<vk::DescriptorPoolSize,1>{  // pPoolSizes
					vk::DescriptorPoolSize(
						vk::DescriptorType::eStorageBuffer,  // type
						4  // descriptorCount
					),
				}.data()
			)
		);

	// descriptor set layout
	_drawableDescriptorSetLayout=
		_device->createDescriptorSetLayout(
			vk::DescriptorSetLayoutCreateInfo(  // createInfo
				vk::DescriptorSetLayoutCreateFlags(),  // flags
				4,  // bindingCount
				array<vk::DescriptorSetLayoutBinding,4>{  // pBindings
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
				}.data()
			)
		);

	// allocate and update descriptor set
	_drawableDescriptorSet=
		_device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				_drawableDescriptorPool,  // descriptorPool
				1,  // descriptorSetCount
				&_drawableDescriptorSetLayout  // pSetLayouts
			)
		)[0];

	// processDrawables shader and pipeline stuff
	_drawableShader=
		_device->createShaderModule(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(processDrawablesShaderSpirv),  // codeSize
				processDrawablesShaderSpirv  // pCode
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
	_drawablePipelineLayout=
		_device->createPipelineLayout(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				1,       // setLayoutCount
				&_drawableDescriptorSetLayout, // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			}
		);
	_drawablePipeline=
		_device->createComputePipeline(
			_pipelineCache,  // pipelineCache
			vk::ComputePipelineCreateInfo(  // createInfo
				vk::PipelineCreateFlags(),  // flags
				vk::PipelineShaderStageCreateInfo(  // stage
					vk::PipelineShaderStageCreateFlags(),  // flags
					vk::ShaderStageFlagBits::eCompute,  // stage
					_drawableShader,  // module
					"main",  // pName
					nullptr  // pSpecializationInfo
				),
				_drawablePipelineLayout,  // layout
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
}


void Renderer::finalize()
{
	if(_device==nullptr)
		return;

	assert((_emptyStorage==nullptr||_emptyStorage->allocationManager().numIDs()==1) && "Renderer::_emptyStorage is not empty. It is a programmer error to allocate anything there. You probably called Geometry::allocAttribs() without specifying AttribSizeList.");

	// clear attrib storages
	_vertexStorages.clear();
	_vertexStorages[AttribSizeList()].emplace_back(this,AttribSizeList()); // create empty VertexStorage for empty AttribSizeList (no attributes)
	_emptyStorage=&_vertexStorages.begin()->second.front();

	// clear allocation managers
	_indexAllocationManager.clear();
	_dataStorageAllocationManager.clear();
	_primitiveSetAllocationManager.clear();

	// delete staging managers
	_indexStagingManagerList.clear_and_dispose([](StagingManager* sm){delete sm;});
	_dataStorageStagingManagerList.clear_and_dispose([](StagingManager* sm){delete sm;});
	_primitiveSetStagingManagerList.clear_and_dispose([](StagingManager* sm){delete sm;});

	// destroy shaders, pipelines,...
	_device->destroy(_drawableShader);
	_device->destroy(_drawableDescriptorPool);
	_device->destroy(_drawableDescriptorSetLayout);
	_device->destroy(_drawablePipelineLayout);
	_device->destroy(_drawablePipeline);
	_device->destroy(_pipelineCache);

	// clean up uploading operations
	_device->destroy(_transientCommandPool);  // no need to destroy commandBuffers as destroying command pool frees all command buffers allocated from the pool

	// destroy fence
	_device->destroy(_fence);

	// destroy buffers
	_device->destroy(_indexBuffer);
	_device->freeMemory(_indexBufferMemory);
	_device->destroy(_dataStorageBuffer);
	_device->freeMemory(_dataStorageMemory);
	_device->destroy(_primitiveSetBuffer);
	_device->freeMemory(_primitiveSetBufferMemory);
	_device->destroy(_drawableBuffer);
	_device->freeMemory(_drawableBufferMemory);
	_drawableBufferSize=0;
	_device->destroy(_drawableStagingBuffer);
	_device->freeMemory(_drawableStagingMemory);
	_device->destroy(_matrixListControlBuffer);
	_device->freeMemory(_matrixListControlBufferMemory);
	_device->destroy(_drawIndirectBuffer);
	_device->freeMemory(_drawIndirectMemory);

	_device=nullptr;
}


void Renderer::leakResources()
{
	// intentionally avoid all destructors
	_vertexStorages.swap(*new decltype(_vertexStorages));
	_device=nullptr;
}


size_t Renderer::prepareSceneRendering(StateSet& stateSetRoot)
{
	// prepare recording
	// and get number of drawables we will render
	size_t numDrawables=stateSetRoot.prepareRecording();

	// reallocate drawable buffer
	// if too small
	if(_drawableBufferSize<numDrawables*sizeof(DrawableGpuData))
	{
		size_t n=size_t(numDrawables*1.2f);  // get extra 20% to avoid frequent reallocations when space needs are growing slowly with time
		_drawableBufferSize=n*sizeof(DrawableGpuData);
		size_t drawIndirectBufferSize=n*sizeof(vk::DrawIndexedIndirectCommand);

		// free previous buffers (if any)
		_device->destroy(_drawableBuffer);
		_device->free(_drawableBufferMemory);
		_device->destroy(_drawableStagingBuffer);
		_device->free(_drawableStagingMemory);
		_device->destroy(_drawIndirectBuffer);
		_device->free(_drawIndirectMemory);

		// alloc new drawable buffer
		_drawableBuffer=
			_device->createBuffer(
				vk::BufferCreateInfo(
					vk::BufferCreateFlags(),      // flags
					_drawableBufferSize,          // size
					vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,  // usage
					vk::SharingMode::eExclusive,  // sharingMode
					0,                            // queueFamilyIndexCount
					nullptr                       // pQueueFamilyIndices
				)
			);
		_drawableBufferMemory=allocateMemory(_drawableBuffer,vk::MemoryPropertyFlagBits::eDeviceLocal);
		_device->bindBufferMemory(
			_drawableBuffer,  // buffer
			_drawableBufferMemory,  // memory
			0  // memoryOffset
		);

		// alloc new drawable staging buffer
		_drawableStagingBuffer=
			_device->createBuffer(
				vk::BufferCreateInfo(
					vk::BufferCreateFlags(),      // flags
					_drawableBufferSize,          // size
					vk::BufferUsageFlagBits::eTransferSrc,  // usage
					vk::SharingMode::eExclusive,  // sharingMode
					0,                            // queueFamilyIndexCount
					nullptr                       // pQueueFamilyIndices
				)
			);
		_drawableStagingMemory=
			allocateMemory(
				_drawableStagingBuffer,  // buffer
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent |
					vk::MemoryPropertyFlagBits::eHostCached  // requiredFlags
			);
		_device->bindBufferMemory(
			_drawableStagingBuffer,  // buffer
			_drawableStagingMemory,  // memory
			0  // memoryOffset
		);
		_drawableStagingData=reinterpret_cast<DrawableGpuData*>(_device->mapMemory(_drawableStagingMemory,0,_drawableBufferSize));

		// alloc new draw indirect buffer
		_drawIndirectBuffer=
			_device->createBuffer(
				vk::BufferCreateInfo(
					vk::BufferCreateFlags(),      // flags
					drawIndirectBufferSize,       // size
					vk::BufferUsageFlagBits::eIndirectBuffer|vk::BufferUsageFlagBits::eStorageBuffer,  // usage
					vk::SharingMode::eExclusive,  // sharingMode
					0,                            // queueFamilyIndexCount
					nullptr                       // pQueueFamilyIndices
				)
			);
		_drawIndirectMemory=allocateMemory(_drawIndirectBuffer,vk::MemoryPropertyFlagBits::eDeviceLocal);
		_device->bindBufferMemory(
			_drawIndirectBuffer,  // buffer
			_drawIndirectMemory,  // memory
			0  // memoryOffset
		);

		// update descriptor with the new buffer
		_device->updateDescriptorSets(
			vk::WriteDescriptorSet(  // descriptorWrites
				_drawableDescriptorSet,  // dstSet
				0,  // dstBinding
				0,  // dstArrayElement
				4,  // descriptorCount
				vk::DescriptorType::eStorageBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,4>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						_primitiveSetBuffer,  // buffer
						0,  // offset
						VK_WHOLE_SIZE  // range
					),
					vk::DescriptorBufferInfo(
						_drawableBuffer,  // buffer
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
				}.data(),
				nullptr  // pTexelBufferView
			),
			nullptr  // descriptorCopies
		);
	}

	return numDrawables;
}


void Renderer::recordDrawableProcessing(vk::CommandBuffer commandBuffer,size_t numDrawables)
{
	if(numDrawables==0)
		return;

	// fill StateSet buffer with content
	_device->cmdCopyBuffer(
		commandBuffer,  // commandBuffer
		_drawableStagingBuffer,  // srcBuffer
		_drawableBuffer,  // dstBuffer
		vk::BufferCopy(
			0,  // srcOffset
			0,  // dstOffset
			numDrawables*sizeof(DrawableGpuData)  // size
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
	_device->cmdBindPipeline(commandBuffer,vk::PipelineBindPoint::eCompute,drawablePipeline());
	_device->cmdBindDescriptorSets(
		commandBuffer,  // commandBuffer
		vk::PipelineBindPoint::eCompute,  // pipelineBindPoint
		drawablePipelineLayout(),  // layout
		0,  // firstSet
		drawableDescriptorSet(),  // descriptorSets
		nullptr  // dynamicOffsets
	);
	_device->cmdDispatch(commandBuffer,uint32_t(numDrawables),1,1);
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


void Renderer::recordSceneRendering(vk::CommandBuffer commandBuffer,StateSet& stateSetRoot,vk::RenderPass renderPass,
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
	size_t drawableCounter=0;
	stateSetRoot.recordToCommandBuffer(commandBuffer,drawableCounter);
	assert(drawableCounter<=_drawableBufferSize/sizeof(DrawableGpuData) && "Buffer overflow. This should not happen.");

	// end render pass
	_device->cmdEndRenderPass(commandBuffer);
}


VertexStorage* Renderer::getOrCreateVertexStorage(const AttribSizeList& attribSizeList)
{
	std::list<VertexStorage>& vertexStorageList=_vertexStorages[attribSizeList];
	if(vertexStorageList.empty())
		vertexStorageList.emplace_front(this,attribSizeList);
	return &vertexStorageList.front();
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


void Renderer::executeCopyOperations()
{
	// start recording
	_device->beginCommandBuffer(
		_uploadingCommandBuffer,  // commandBuffer
		vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,  // flags
			nullptr  // pInheritanceInfo
		)
	);

	// record command buffer
	for(auto& mapItem : _vertexStorages)
		for(VertexStorage& vs : mapItem.second) {
			StagingManagerList& sml=vs.stagingManagerList();
			for(StagingManager& sm : sml)
				sm.record(_uploadingCommandBuffer);
		}
	for(StagingManager& sm : _indexStagingManagerList)        sm.record(_uploadingCommandBuffer);
	for(StagingManager& sm : _dataStorageStagingManagerList)  sm.record(_uploadingCommandBuffer);
	for(StagingManager& sm : _primitiveSetStagingManagerList) sm.record(_uploadingCommandBuffer);

	// end recording
	_device->endCommandBuffer(_uploadingCommandBuffer);

	// submit command buffer
	_device->queueSubmit(
		_graphicsQueue,  // queue
		vk::SubmitInfo(  // submits (vk::ArrayProxy)
			0,nullptr,nullptr,           // waitSemaphoreCount,pWaitSemaphores,pWaitDstStageMask
			1,&_uploadingCommandBuffer,  // commandBufferCount,pCommandBuffers
			0,nullptr                    // signalSemaphoreCount,pSignalSemaphores
		),
		_fence  // fence
	);

	// wait for work to complete
	vk::Result r=_device->waitForFences(
		_fence,        // fences (vk::ArrayProxy)
		VK_TRUE,       // waitAll
		uint64_t(3e9)  // timeout (3s)
	);
	if(r!=vk::Result::eSuccess) {
		if(r==vk::Result::eTimeout)
			throw std::runtime_error("GPU timeout. Task is probably hanging.");
		throw std::runtime_error("vk::Device::waitForFences() returned strange success code.");	 // error codes are already handled by throw inside waitForFences()
	}

	// dispose staging buffers and staging managers
	auto disposeStagingManagers=
		[](StagingManagerList& l,ArrayAllocationManager& m) {
			for(StagingManager& sm : l) {
				assert(m[sm.allocationID()].stagingManager==&sm && "Broken data integrity.");
				m[sm.allocationID()].stagingManager=nullptr;
			}
			l.clear_and_dispose([](StagingManager* sm){delete sm;});
		};
	for(auto& mapItem : _vertexStorages)
		for(VertexStorage& vs : mapItem.second) {
			disposeStagingManagers(vs.stagingManagerList(),vs.allocationManager());
		}
	disposeStagingManagers(_indexStagingManagerList,_indexAllocationManager);
	disposeStagingManagers(_dataStorageStagingManagerList,_dataStorageAllocationManager);
	disposeStagingManagers(_primitiveSetStagingManagerList,_primitiveSetAllocationManager);
}


StagingBuffer& Renderer::createIndexStagingBuffer(Geometry& g)
{
	ArrayAllocation& a=indexAllocation(g);
	StagingManager& sm=StagingManager::getOrCreate(a,g.indexDataID(),indexStagingManagerList());
	return
		sm.createStagingBuffer(
			this,  // renderer
			_indexBuffer,  // dstBuffer
			a.startIndex*sizeof(uint32_t),  // dstOffset
			a.numItems*sizeof(uint32_t)  // size
		);
}


StagingBuffer& Renderer::createIndexSubsetStagingBuffer(Geometry& g,size_t firstIndex,size_t numIndices)
{
	ArrayAllocation& a=indexAllocation(g);
	if(firstIndex+numIndices>a.numItems)
		throw std::out_of_range("Renderer::createIndexSubsetStagingBuffer(): Parameter firstIndex and numIndices define range that is not completely inside allocated space of Geometry.");
	StagingManager& sm=StagingManager::getOrCreate(a,g.indexDataID(),indexStagingManagerList());
	return
		sm.createStagingBuffer(
			this,  // renderer
			_indexBuffer,  // dstBuffer
			(a.startIndex+firstIndex)*sizeof(uint32_t),  // dstOffset
			numIndices*sizeof(uint32_t)  // size
		);
}


void Renderer::uploadIndices(Geometry& g,std::vector<uint32_t>& indexData)
{
	if(indexData.empty()) return;

	// create StagingBuffer and submit it
	StagingBuffer& sb=createIndexStagingBuffer(g);
	if(indexData.size()*sizeof(uint32_t)!=sb.size())
		throw std::out_of_range("Renderer::uploadIndices(): size of indexData must match allocated space for indices inside Geometry.");
	memcpy(sb.data(),indexData.data(),sb.size());
}


void Renderer::uploadIndicesSubset(Geometry& g,std::vector<uint32_t>& indexData,size_t firstIndex)
{
	if(indexData.empty()) return;

	// create StagingBuffer and submit it
	StagingBuffer& sb=createIndexSubsetStagingBuffer(g,firstIndex,indexData.size());
	memcpy(sb.data(),indexData.data(),sb.size());
}


StagingBuffer& Renderer::createDataStorageStagingBuffer(uint32_t id)
{
	ArrayAllocation& a=dataStorageAllocation(id);
	StagingManager& sm=StagingManager::getOrCreate(a,id,dataStorageStagingManagerList());
	return
		sm.createStagingBuffer(
			this,  // renderer
			_dataStorageBuffer,  // dstBuffer
			a.startIndex,  // dstOffset
			a.numItems  // size
		);
}


StagingBuffer& Renderer::createDataStorageSubsetStagingBuffer(uint32_t id,size_t offset,size_t size)
{
	ArrayAllocation& a=dataStorageAllocation(id);
	if(offset+size>a.numItems)
		throw std::out_of_range("Renderer::createDataStorageSubsetStagingBuffer(): Parameter offset and size define range that is not completely inside allocated space of id allocation.");
	StagingManager& sm=StagingManager::getOrCreate(a,id,dataStorageStagingManagerList());
	return
		sm.createStagingBuffer(
			this,  // renderer
			_dataStorageBuffer,  // dstBuffer
			a.startIndex+offset,  // dstOffset
			size  // size
		);
}


void Renderer::uploadDataStorage(uint32_t id,const void* data,size_t size)
{
	if(size==0) return;

	// create StagingBuffer and submit it
	StagingBuffer& sb=createDataStorageStagingBuffer(id);
	if(size!=sb.size())
		throw std::out_of_range("Renderer::uploadDataStorage(): Size of allocation given by id and size of data does not match.");
	memcpy(sb.data(),data,size);
}


void Renderer::uploadDataStorageSubset(uint32_t id,const void* data,size_t size,size_t offset)
{
	if(size==0) return;

	// create StagingBuffer and submit it
	StagingBuffer& sb=createDataStorageSubsetStagingBuffer(id,offset,size);
	memcpy(sb.data(),data,size);
}


StagingBuffer& Renderer::createPrimitiveSetStagingBuffer(Geometry& g)
{
	ArrayAllocation& a=primitiveSetAllocation(g);
	StagingManager& sm=StagingManager::getOrCreate(a,g.primitiveSetDataID(),primitiveSetStagingManagerList());
	return
		sm.createStagingBuffer(
			this,  // renderer
			_primitiveSetBuffer,  // dstBuffer
			a.startIndex*sizeof(PrimitiveSetGpuData),  // dstOffset
			a.numItems*sizeof(PrimitiveSetGpuData)  // size
		);
}


StagingBuffer& Renderer::createPrimitiveSetSubsetStagingBuffer(Geometry& g,size_t firstPrimitiveSet,size_t numPrimitiveSets)
{
	ArrayAllocation& a=primitiveSetAllocation(g);
	if(firstPrimitiveSet+numPrimitiveSets>a.numItems)
		throw std::out_of_range("Renderer::createPrimitiveSetSubsetStagingBuffer(): Parameter firstPrimitiveSet and numPrimitiviveSets define range that is not completely inside allocated space of primitiveSet allocation.");
	StagingManager& sm=StagingManager::getOrCreate(a,g.primitiveSetDataID(),primitiveSetStagingManagerList());
	return
		sm.createStagingBuffer(
			this,  // renderer
			_primitiveSetBuffer,  // dstBuffer
			(a.startIndex+firstPrimitiveSet)*sizeof(PrimitiveSetGpuData),  // dstOffset
			numPrimitiveSets*sizeof(PrimitiveSetGpuData)  // size
		);
}


void Renderer::uploadPrimitiveSets(Geometry& g,std::vector<PrimitiveSetGpuData>& primitiveSetData)
{
	if(primitiveSetData.empty()) return;

	// create StagingBuffer and submit it
	StagingBuffer& sb=createPrimitiveSetStagingBuffer(g);
	if(primitiveSetData.size()*sizeof(PrimitiveSetGpuData)!=sb.size())
		throw std::out_of_range("Renderer::uploadPrimitiveSets(): size of primitiveSetData must match allocated space for primitiveSets inside Geometry.");
	memcpy(sb.data(),primitiveSetData.data(),sb.size());
}


void Renderer::uploadPrimitiveSetsSubset(Geometry& g,std::vector<PrimitiveSetGpuData>& primitiveSetData,size_t firstPrimitiveSetIndex)
{
	if(primitiveSetData.empty()) return;

	// create StagingBuffer and submit it
	StagingBuffer& sb=createPrimitiveSetSubsetStagingBuffer(g,firstPrimitiveSetIndex,primitiveSetData.size());
	memcpy(sb.data(),primitiveSetData.data(),sb.size());
}
