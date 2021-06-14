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

// global variables
Renderer* Renderer::_defaultRenderer = nullptr;
size_t Renderer::initialVertexStorageCapacity = 131072;
size_t Renderer::initialIndexStorageCapacity = 131072;
size_t Renderer::initialDataStorageCapacity = 524288;
size_t Renderer::initialPrimitiveSetStorageCapacity = 16384;

// static functions
static inline double getCpuTimestampPeriod();



Renderer::Renderer(bool makeDefault)
	: _device(nullptr)
	, _graphicsQueueFamily(0xffffffff)
	, _emptyStorage(nullptr)
	, _indexAllocationManager(initialIndexStorageCapacity,0)  // set initial capacity and set size of null object on index 0 to zero
	, _dataStorageAllocationManager(initialDataStorageCapacity,0)  // set initial capacity and set size of null object on index 0 to zero
	, _primitiveSetAllocationManager(initialPrimitiveSetStorageCapacity,0)  // set initial capacity and set size of null object on index 0 to zero
{
	_vertexStorages[AttribSizeList()].emplace_back(this,AttribSizeList(),0); // create empty VertexStorage for empty AttribSizeList (no attributes)
	_emptyStorage=&_vertexStorages.begin()->second.front();
	if(makeDefault)
		Renderer::set(this);
}

Renderer::Renderer(VulkanDevice& device,VulkanInstance& instance,vk::PhysicalDevice physicalDevice,
                   uint32_t graphicsQueueFamily,bool makeDefault)
	: _device(nullptr)
	, _graphicsQueueFamily(graphicsQueueFamily)
	, _emptyStorage(nullptr)
	, _indexAllocationManager(initialIndexStorageCapacity,0)  // set intial capacity and set size of null object on index 0 to zero
	, _dataStorageAllocationManager(initialDataStorageCapacity,0)  // set initial capacity and set size of null object on index 0 to zero
	, _primitiveSetAllocationManager(initialPrimitiveSetStorageCapacity,0)  // set initial capacity and set size of null object on index 0 to zero
{
	_vertexStorages[AttribSizeList()].emplace_back(this,AttribSizeList(),0); // create empty VertexStorage for empty AttribSizeList (no attributes)
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
	vk::PhysicalDeviceProperties p=instance.getPhysicalDeviceProperties(physicalDevice);
	vk::DeviceSize nonCoherentAtomSize=p.limits.nonCoherentAtomSize;
	nonCoherentAtom_addition=nonCoherentAtomSize-1;
	nonCoherentAtom_mask=~nonCoherentAtom_addition;
	if((nonCoherentAtomSize&nonCoherentAtom_addition)!=0)  // is it power of two?
		throw std::runtime_error("Platform problem: nonCoherentAtomSize is not power of two.");

	// timestamp periods
	_gpuTimestampPeriod=p.limits.timestampPeriod*1e-9f;
	_cpuTimestampPeriod=getCpuTimestampPeriod();

	// index buffer
	_indexBuffer=
		_device->createBuffer(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				_indexAllocationManager.capacity()*sizeof(uint32_t),  // size
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
				_dataStorageAllocationManager.capacity(),  // size
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
				_primitiveSetAllocationManager.capacity()*sizeof(PrimitiveSetGpuData),  // size
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

	// precompiledCommandPool and readTimestampCommandBuffer
	_precompiledCommandPool=
		_device->createCommandPool(
			vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlagBits::eResetCommandBuffer,  // flags
				_graphicsQueueFamily  // queueFamilyIndex
			)
		);
	_readTimestampQueryPool=
		_device->createQueryPool(
			vk::QueryPoolCreateInfo(
				vk::QueryPoolCreateFlags(),  // flags
				vk::QueryType::eTimestamp,  // queryType
				1,  // queryCount
				vk::QueryPipelineStatisticFlags()  // pipelineStatistics
			)
		);
	_readTimestampCommandBuffer=
		_device->allocateCommandBuffers(
			vk::CommandBufferAllocateInfo(
				_precompiledCommandPool,             // commandPool
				vk::CommandBufferLevel::ePrimary,  // level
				1                                  // commandBufferCount
			)
		)[0];
	_device->beginCommandBuffer(
		_readTimestampCommandBuffer,  // commandBuffer
		vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlags(),  // flags
			nullptr  // pInheritanceInfo
		)
	);
	_device->cmdWriteTimestamp(
		_readTimestampCommandBuffer, // commandBuffer
		vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
		_readTimestampQueryPool,  // queryPool
		0  // query
	);
	_device->endCommandBuffer(_readTimestampCommandBuffer);
}


void Renderer::finalize()
{
	if(_device==nullptr)
		return;

	assert((_emptyStorage==nullptr||_emptyStorage->allocationManager().numIDs()==1) && "Renderer::_emptyStorage is not empty. It is a programmer error to allocate anything there. You probably called Geometry::allocAttribs() without specifying AttribSizeList.");

	// clear attrib storages
	_vertexStorages.clear();
	_vertexStorages[AttribSizeList()].emplace_back(this,AttribSizeList(),0); // create empty VertexStorage for empty AttribSizeList (no attributes)
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

	// clean up precompiled command buffers
	_device->destroy(_precompiledCommandPool);  // no need to destroy commandBuffers as destroying command pool frees all command buffers allocated from the pool
	_device->destroy(_readTimestampQueryPool);

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


size_t Renderer::beginFrame()
{
	_frameNumber++;

	// delete completedStats that were not retrieved by the user
	_completedFrameInfoList.clear();

	// process _inProgressStats if they are ready
	if(!_inProgressFrameInfoList.empty())
		_completedFrameInfoList = getFrameInfos();

	// collect frame statistics
	if(collectFrameInfo()) {

		// prepare stats processing
		_timestampIndex = 0;
		_inProgressFrameInfoList.emplace_back();
		auto& [stats,timestampPool] = _inProgressFrameInfoList.back();
		stats.frameNumber = _frameNumber;
			
		// get calibrated timestamps
		if(_useCalibratedTimestamps) {
			array<uint64_t,2> ts;
			uint64_t maxDeviation;
			_device->getCalibratedTimestampsEXT(
				2,  // timestampCount
				array{  // pTimestampInfos
					vk::CalibratedTimestampInfoEXT(vk::TimeDomainEXT::eDevice),
					vk::CalibratedTimestampInfoEXT(_timestampHostTimeDomain),
				}.data(),
				ts.data(),  // pTimestamps
				&maxDeviation  // pMaxDeviation
			);
			stats.beginFrameGpu = ts[0];
			stats.beginFrameCpu = ts[1];
		}
		else {
			stats.beginFrameGpu = getGpuTimestamp();
			stats.beginFrameCpu = getCpuTimestamp();
		}

		// create timestamp pool
		timestampPool=
			_device->createQueryPoolUnique(
				vk::QueryPoolCreateInfo(
					vk::QueryPoolCreateFlags(),  // flags
					vk::QueryType::eTimestamp,  // queryType
					FrameInfo::gpuTimestampPoolSize,  // queryCount
					vk::QueryPipelineStatisticFlags()  // pipelineStatistics
				)
			);
	}

	return _frameNumber;
}


void Renderer::beginRecording(vk::CommandBuffer commandBuffer)
{
	// begin command buffer recording
	_device->beginCommandBuffer(
		commandBuffer,  // commandBuffer
		vk::CommandBufferBeginInfo(  // beginInfo
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,  // flags
			nullptr  // pInheritanceInfo
		)
	);

	// schedule write of gpu timestamp
	if(collectFrameInfo()) {
		_device->cmdResetQueryPool(
			commandBuffer,  // commandBuffer
			_inProgressFrameInfoList.back().timestampPool.get(),  // queryPool
			0,  // firstQuery
			FrameInfo::gpuTimestampPoolSize  // queryCount
		);
		_device->cmdWriteTimestamp(
			commandBuffer,  // commandBuffer
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			_inProgressFrameInfoList.back().timestampPool.get(),  // queryPool
			_timestampIndex++  // query
		);
	}
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


void Renderer::endRecording(vk::CommandBuffer commandBuffer)
{
	// schedule write of gpu timestamp
	if(collectFrameInfo())
		_device->cmdWriteTimestamp(
			commandBuffer,  // commandBuffer
			vk::PipelineStageFlagBits::eBottomOfPipe,  // pipelineStage
			_inProgressFrameInfoList.back().timestampPool.get(),  // queryPool
			_timestampIndex++  // query
		);

	// finish recording of command buffer
	_device->endCommandBuffer(commandBuffer);
}


void Renderer::endFrame()
{
	if(collectFrameInfo()) {

		// write cpu timestamp
		auto& [stats,timestampPool] = _inProgressFrameInfoList.back();
		assert(stats.frameNumber==_frameNumber && "Do not start new frame until the recording of previous one is finished.");
		assert(_timestampIndex==FrameInfo::gpuTimestampPoolSize && "Wrong number of gpu timestamps.");
		if(_useCalibratedTimestamps) {
			uint64_t ts;
			uint64_t maxDeviation;
			_device->getCalibratedTimestampsEXT(
				1,  // timestampCount
				array{ vk::CalibratedTimestampInfoEXT(_timestampHostTimeDomain) }.data(),  // pTimestampInfos
				&ts,  // pTimestamps
				&maxDeviation  // pMaxDeviation
			);
			stats.endFrameCpu = ts;
		}
		else
			stats.endFrameCpu = getCpuTimestamp();
	}
}


VertexStorage* Renderer::getOrCreateVertexStorage(const AttribSizeList& attribSizeList)
{
	std::list<VertexStorage>& vertexStorageList=_vertexStorages[attribSizeList];
	if(vertexStorageList.empty())
		vertexStorageList.emplace_front(this,attribSizeList,initialVertexStorageCapacity);
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
	_device->resetFences(_fence);
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


void Renderer::setCollectFrameInfo(bool on, bool useCalibratedTimestamps)
{
#if _WIN32
	setCollectFrameInfo(on, useCalibratedTimestamps, vk::TimeDomainEXT::eQueryPerformanceCounter);
#else
	setCollectFrameInfo(on, useCalibratedTimestamps, vk::TimeDomainEXT::eClockMonotonicRaw);
#endif
}


void Renderer::setCollectFrameInfo(bool on, bool useCalibratedTimestamps, vk::TimeDomainEXT timestampHostTimeDomain)
{
	_collectFrameInfo = on;
	_useCalibratedTimestamps = useCalibratedTimestamps;
	_timestampHostTimeDomain = timestampHostTimeDomain;
}


list<FrameInfo> Renderer::getFrameInfos()
{
	// _completedStats go to the result l
	list<FrameInfo> l;
	l.swap(_completedFrameInfoList);

	// completed _inProgressStats go to the result l
	// (we return them "in order", so test only the one
	// in the front of the _inProgressStats)
	while(!_inProgressFrameInfoList.empty()) {

		// query for results
		auto& [stats,timestampPool] = _inProgressFrameInfoList.front();
		array<uint64_t, FrameInfo::gpuTimestampPoolSize> timestamps;
		vk::Result r =
			_device->getQueryPoolResults(
				timestampPool.get(),  // queryPool
				0,                    // firstQuery
				FrameInfo::gpuTimestampPoolSize,  // queryCount
				FrameInfo::gpuTimestampPoolSize*sizeof(uint64_t),  // dataSize
				timestamps.data(),    // pData
				sizeof(uint64_t),     // stride
				vk::QueryResultFlagBits::e64  // flags
			);

		// if not ready, just return what we collected until now
		if(r == vk::Result::eNotReady)
			return l;

		// if success, append the result in l
		// and go to the next _inProgressStats item
		if(r == vk::Result::eSuccess) {
			stats.beginRenderingGpu = timestamps[0];
			stats.endRenderingGpu = timestamps[1];
			l.emplace_back(stats);
			_inProgressFrameInfoList.pop_front();
			continue;
		}

		// handle other unexpected and currently undocumented success values by return
		return l;
	}

	// handle reaching the empty _inProgressStats
	return l;
}


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN  // exclude rarely-used services inclusion by windows.h; this speeds up compilation and avoids some compilation problems
#include <windows.h>  // we include windows.h only at the end of file to avoid compilation problems; windows.h define MemoryBarrier, near, far and many other problematic macros
#endif
static inline double getCpuTimestampPeriod()
{
#ifdef _WIN32
	LARGE_INTEGER f;
	QueryPerformanceFrequency(&f);
	return 1.0 / f.QuadPart;
#else
	return 1e-9;  // on Linux, we use clock_gettime()
#endif
}


uint64_t Renderer::getCpuTimestamp() const
{
#ifdef _WIN32
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return uint64_t(counter.QuadPart);
#else
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC_RAW, &tv);
	return tv.tv_nsec + tv.tv_sec*1000000000ull;
#endif
}


uint64_t Renderer::getGpuTimestamp() const
{
	// submit command buffer
	_device->queueSubmit(
		_graphicsQueue,  // queue
		vk::SubmitInfo(  // submits (vk::ArrayProxy)
			0,nullptr,nullptr,               // waitSemaphoreCount,pWaitSemaphores,pWaitDstStageMask
			1,&_readTimestampCommandBuffer,  // commandBufferCount,pCommandBuffers
			0,nullptr                        // signalSemaphoreCount,pSignalSemaphores
		),
		_fence  // fence
	);

	// wait for work to complete
	vk::Result r=_device->waitForFences(
		_fence,        // fences (vk::ArrayProxy)
		VK_TRUE,       // waitAll
		uint64_t(3e9)  // timeout (3s)
	);
	_device->resetFences(_fence);
	if(r!=vk::Result::eSuccess) {
		if(r==vk::Result::eTimeout)
			throw std::runtime_error("GPU timeout. Task is probably hanging.");
		throw std::runtime_error("vk::Device::waitForFences() returned strange success code.");	 // error codes are already handled by throw inside waitForFences()
	}

	// read timestamp 
	uint64_t t;
	r=_device->getQueryPoolResults(
			_readTimestampQueryPool,  // queryPool
			0,                    // firstQuery
			1,  // queryCount
			1*sizeof(uint64_t),  // dataSize
			&t,    // pData
			sizeof(uint64_t),     // stride
			vk::QueryResultFlagBits::e64  // flags
		);

	// if not ready, something is wrong with Vulkan
	if(r == vk::Result::eNotReady)
		return 0;

	// if error for some strange reason, return 0
	if(r != vk::Result::eSuccess)
		return 0;

	// return timestamp
	return t;
}
