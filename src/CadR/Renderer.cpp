#include <CadR/Renderer.h>
#include <CadR/DataStorage.h>
#include <CadR/Exceptions.h>
#include <CadR/ImageStorage.h>
#include <CadR/StagingManager.h>
#include <CadR/StateSet.h>
#include <CadR/TransferResources.h>
#include <CadR/VulkanDevice.h>
#include <CadR/VulkanInstance.h>

using namespace std;
using namespace CadR;

// required features
// (Currently, CadR::Renderer requires multiDrawIndirect, shaderInt64, shaderDrawParameters and bufferDeviceAddress features.)
Renderer::RequiredFeaturesStructChain Renderer::_requiredFeatures;
namespace CadR {
	struct RendererStaticInitializer {
		RendererStaticInitializer() {
			Renderer::_requiredFeatures.get<vk::PhysicalDeviceFeatures2>().features.multiDrawIndirect = true;
			Renderer::_requiredFeatures.get<vk::PhysicalDeviceFeatures2>().features.shaderInt64 = true;
			Renderer::_requiredFeatures.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters = true;
			Renderer::_requiredFeatures.get<vk::PhysicalDeviceVulkan12Features>().bufferDeviceAddress = true;
		}
	};
}
static CadR::RendererStaticInitializer initializer;

// shader code in SPIR-V binary
static const uint32_t processDrawablesL1ShaderSpirv[]={
#include "shaders/processDrawables-l1.comp.spv"
};
static const uint32_t processDrawablesL2ShaderSpirv[]={
#include "shaders/processDrawables-l2.comp.spv"
};
static const uint32_t processDrawablesL3ShaderSpirv[]={
#include "shaders/processDrawables-l3.comp.spv"
};

// global variables
Renderer* Renderer::_defaultRenderer = nullptr;

// static functions
static inline double getCpuTimestampPeriod();



Renderer::Renderer(bool makeDefault) noexcept
	: _device(nullptr)
	, _graphicsQueueFamily(0xffffffff)
	, _stagingManager(*this)
	, _dataStorage(*this)
	, _imageStorage(*this)
{
	// make Renderer default
	if(makeDefault)
		Renderer::set(*this);
}

Renderer::Renderer(VulkanDevice& device, VulkanInstance& instance, vk::PhysicalDevice physicalDevice,
                   uint32_t graphicsQueueFamily, bool makeDefault)
	: _device(nullptr)
	, _graphicsQueueFamily(graphicsQueueFamily)
	, _stagingManager(*this)
	, _dataStorage(*this)
	, _imageStorage(*this)
{
	// init
	init(device, instance, physicalDevice, graphicsQueueFamily, makeDefault);
}


Renderer::~Renderer()
{
	finalize();
	if(_defaultRenderer == this)
		_defaultRenderer = nullptr;
}


void Renderer::init(VulkanDevice& device, VulkanInstance& instance, vk::PhysicalDevice physicalDevice,
                    uint32_t graphicsQueueFamily, bool makeDefault)
{
	if(_device)
		finalize();

	if(makeDefault)
		Renderer::set(*this);

	_device = &device;
	_graphicsQueueFamily = graphicsQueueFamily;
	_graphicsQueue = _device->getQueue(_graphicsQueueFamily, 0);
	_memoryProperties = instance.getPhysicalDeviceMemoryProperties(physicalDevice);

	// init storages
	_dataStorage.init(_stagingManager);
	_imageStorage.init(_stagingManager, _memoryProperties.memoryTypeCount);

	// _standardBufferAlignment
	_standardBufferAlignment =
		_device->getBufferMemoryRequirements(
			_device->createBufferUnique(
				vk::BufferCreateInfo(
					vk::BufferCreateFlags(),      // flags
					1,                            // size
					vk::BufferUsageFlagBits::eVertexBuffer|vk::BufferUsageFlagBits::eIndexBuffer|  // usage
						vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|
						vk::BufferUsageFlagBits::eTransferDst,
					vk::SharingMode::eExclusive,  // sharingMode
					0,                            // queueFamilyIndexCount
					nullptr                       // pQueueFamilyIndices
				)
			).get())
		.alignment;
	if((_standardBufferAlignment&(_standardBufferAlignment-1)) != 0)  // is it power of two? -> this is guaranteed to be true by Vulkan spec since somewhere between 1.0.20 and 1.0.36
		throw std::runtime_error("Platform problem: standardBufferAlignment is not power of two.");

	// nonCoherentAtomSize
	vk::PhysicalDeviceProperties p = instance.getPhysicalDeviceProperties(physicalDevice);
	vk::DeviceSize nonCoherentAtomSize = p.limits.nonCoherentAtomSize;
	_nonCoherentAtom_addition = nonCoherentAtomSize-1;
	_nonCoherentAtom_mask = ~_nonCoherentAtom_addition;
	if((nonCoherentAtomSize & _nonCoherentAtom_addition) != 0)  // is it power of two?
		throw std::runtime_error("Platform problem: nonCoherentAtomSize is not power of two.");

	// timestamp periods
	_gpuTimestampPeriod = p.limits.timestampPeriod * 1e-9f;
	_cpuTimestampPeriod = getCpuTimestampPeriod();

	// FrameInfo structures
	_inProgressFrameInfo = {};
	_inProgressFrameInfo.frameNumber = ~size_t(0);
	_completedFrameInfo = {};
	_completedFrameInfo.frameNumber = ~size_t(0);

	// create general purpose fence
	_fence =
		_device->createFence(vk::FenceCreateInfo{vk::FenceCreateFlags()});

	// descriptor pool
	_descriptorPool =
		_device->createDescriptorPool(
			vk::DescriptorPoolCreateInfo(
				vk::DescriptorPoolCreateFlags(),  // flags
				2,  // maxSets
				1,  // poolSizeCount
				array<vk::DescriptorPoolSize, 1>{  // pPoolSizes
					vk::DescriptorPoolSize(
						vk::DescriptorType::eStorageBuffer,  // type
						5  // descriptorCount
					),
				}.data()
			)
		);

	// processDrawables shader and pipeline stuff
	_processDrawablesShaderList[0] =
		_device->createShaderModule(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(processDrawablesL1ShaderSpirv),  // codeSize
				processDrawablesL1ShaderSpirv  // pCode
			)
		);
	_processDrawablesShaderList[1] =
		_device->createShaderModule(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(processDrawablesL2ShaderSpirv),  // codeSize
				processDrawablesL2ShaderSpirv  // pCode
			)
		);
	_processDrawablesShaderList[2] =
		_device->createShaderModule(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(processDrawablesL3ShaderSpirv),  // codeSize
				processDrawablesL3ShaderSpirv  // pCode
			)
		);
	_pipelineCache =
		_device->createPipelineCache(
			vk::PipelineCacheCreateInfo(
				vk::PipelineCacheCreateFlags(),  // flags
				0,       // initialDataSize
				nullptr  // pInitialData
			)
		);
	_processDrawablesPipelineLayout =
		_device->createPipelineLayout(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				0,        // setLayoutCount
				nullptr,  // pSetLayouts
				1,  // pushConstantRangeCount
				&(const vk::PushConstantRange&)vk::PushConstantRange{  // pPushConstantRanges
					vk::ShaderStageFlagBits::eCompute,  // stageFlags
					0,  // offset
					4*sizeof(uint64_t)  // size
				}
			}
		);
	for(size_t i=0; i<3; i++)
	{
		_processDrawablesPipelineList[i] =
			_device->createComputePipeline(
				//_pipelineCache,  // pipelineCache
				nullptr,
				vk::ComputePipelineCreateInfo(  // createInfo
					vk::PipelineCreateFlagBits::eDispatchBase,  // flags
					vk::PipelineShaderStageCreateInfo(  // stage
						vk::PipelineShaderStageCreateFlags(),  // flags
						vk::ShaderStageFlagBits::eCompute,  // stage
						_processDrawablesShaderList[i],  // module
						"main",  // pName
						nullptr  // pSpecializationInfo
					),
					_processDrawablesPipelineLayout,  // layout
					nullptr,  // basePipelineHandle
					-1  // basePipelineIndex
				)
			);
	}

	// transientCommandPool and uploadingCommandBuffer
	_transientCommandPool =
		_device->createCommandPool(
			vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlagBits::eTransient|vk::CommandPoolCreateFlagBits::eResetCommandBuffer,  // flags
				_graphicsQueueFamily  // queueFamilyIndex
			)
		);
	_uploadingCommandBuffer =
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
	_device->cmdResetQueryPool(
		_readTimestampCommandBuffer,  // commandBuffer
		_readTimestampQueryPool,  // queryPool
		0,  // firstQuery
		1  // queryCount
	);
	_device->cmdWriteTimestamp(
		_readTimestampCommandBuffer,  // commandBuffer
		vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
		_readTimestampQueryPool,  // queryPool
		0  // query
	);
	_device->endCommandBuffer(_readTimestampCommandBuffer);

	_frameInfoTimestampPool =
		_device->createQueryPool(
			vk::QueryPoolCreateInfo(
				vk::QueryPoolCreateFlags(),  // flags
				vk::QueryType::eTimestamp,  // queryType
				FrameInfo::gpuTimestampPoolSize,  // queryCount
				vk::QueryPipelineStatisticFlags()  // pipelineStatistics
			)
		);
}


void Renderer::finalize()
{
	if(_device == nullptr)
		return;

	// destroy shaders, pipelines,...
	for(size_t i=0; i<_processDrawablesShaderList.size(); i++) {
		_device->destroy(_processDrawablesShaderList[i]);
		_processDrawablesShaderList[i] = nullptr;
	}
	_device->destroy(_descriptorPool);
	_descriptorPool = nullptr;
	_device->destroy(_processDrawablesPipelineLayout);
	_processDrawablesPipelineLayout = nullptr;
	for(size_t i=0; i<_processDrawablesPipelineList.size(); i++) {
		_device->destroy(_processDrawablesPipelineList[i]);
		_processDrawablesPipelineList[i] = nullptr;
	}
	_device->destroy(_pipelineCache);
	_pipelineCache = nullptr;
	_device->destroy(_frameInfoTimestampPool);
	_frameInfoTimestampPool = nullptr;

	// clean up uploading operations
	_device->destroy(_transientCommandPool);  // no need to destroy commandBuffers as destroying command pool frees all command buffers allocated from the pool
	_transientCommandPool = nullptr;

	// clean up precompiled command buffers
	_device->destroy(_precompiledCommandPool);  // no need to destroy commandBuffers as destroying command pool frees all command buffers allocated from the pool
	_device->destroy(_readTimestampQueryPool);
	_precompiledCommandPool = nullptr;
	_readTimestampQueryPool = nullptr;

	// destroy fence
	_device->destroy(_fence);
	_fence = nullptr;

	// destroy buffers
	_dataStorage.cleanUp();
	_imageStorage.cleanUp();
	_stagingManager.cleanUp();
	_device->destroy(_drawableBuffer);
	_device->freeMemory(_drawableBufferMemory);
	_drawableBufferSize=0;
	_device->destroy(_drawableStagingBuffer);
	_device->freeMemory(_drawableStagingMemory);
	_device->destroy(_drawIndirectBuffer);
	_device->freeMemory(_drawIndirectMemory);
	_device->destroy(_drawablePointersBuffer);
	_device->freeMemory(_drawablePointersMemory);
	_drawableBuffer = nullptr;
	_drawableBufferMemory = nullptr;
	_drawableStagingBuffer = nullptr;
	_drawableStagingMemory = nullptr;
	_drawIndirectBuffer = nullptr;
	_drawIndirectMemory = nullptr;
	_drawablePointersBuffer = nullptr;
	_drawablePointersMemory = nullptr;

	_device = nullptr;
}


void Renderer::leakResources()
{
	// release storage and staging resources before we assign nullptr to _device
	_dataStorage.cleanUp();
	_imageStorage.cleanUp();
	_stagingManager.cleanUp();

	_device = nullptr;
}


size_t Renderer::beginFrame()
{
	_frameNumber++;

	// optimize amount of staging memory
	_lastFrameUploadBytes = _currentFrameUploadBytes;
	_currentFrameUploadBytes = 0;
	_dataStorage.setStagingDataSizeHint(_lastFrameUploadBytes);

	// collect frame statistics
	if(_collectFrameInfo) {

		// prepare frame info collecting
		_inProgressFrameInfo = {};  // perform zero initialization
		_inProgressFrameInfo.frameNumber = _frameNumber;
		_inProgressFrameInfo.beingCollected = true;

		// get cpu and gpu timestamps
		// (they might be calibrated timestamps if supported)
		tie(_inProgressFrameInfo.cpuBeginFrame, _inProgressFrameInfo.gpuBeginFrame) = getCpuAndGpuTimestamps();

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
	if(_collectFrameInfo) {
		_device->cmdResetQueryPool(
			commandBuffer,  // commandBuffer
			_frameInfoTimestampPool,  // queryPool
			0,  // firstQuery
			FrameInfo::gpuTimestampPoolSize  // queryCount
		);
		_device->cmdWriteTimestamp(
			commandBuffer,  // commandBuffer
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			_frameInfoTimestampPool,  // queryPool
			_inProgressFrameInfo.numTimestamps++  // query
		);
	}
}


size_t Renderer::prepareSceneRendering(StateSet& stateSetRoot)
{
	// prepare recording
	// and get number of drawables we will render
	size_t numDrawables;
	if(_collectFrameInfo == false)
		numDrawables = stateSetRoot.prepareRecording();
	else {
		_inProgressFrameInfo.cpuPrepareRecordingBegin = getCpuTimestamp();
		numDrawables = stateSetRoot.prepareRecording();
		_inProgressFrameInfo.cpuPrepareRecordingEnd = getCpuTimestamp();
	}

	// reallocate drawable buffer
	// if too small
	if(_drawableBufferSize < numDrawables*sizeof(DrawableGpuData))
	{
		size_t n = size_t(numDrawables * 1.2f);  // get extra 20% to avoid frequent reallocations when space needs are growing slowly with time
		if(n < 128)
			n = 128;
		_drawableBufferSize = n * sizeof(DrawableGpuData);
		size_t drawIndirectBufferSize = n * sizeof(vk::DrawIndirectCommand);
		size_t drawablePointersBufferSize = n * drawablePointersRecordSize;

		// free previous buffers (if any)
		// (null needs to be assigned to variables because createBuffer() calls might throw in the case of error)
		_device->destroy(_drawableBuffer);
		_device->free(_drawableBufferMemory);
		_device->destroy(_drawableStagingBuffer);
		_device->free(_drawableStagingMemory);
		_device->destroy(_drawIndirectBuffer);
		_device->free(_drawIndirectMemory);  
		_device->destroy(_drawablePointersBuffer);
		_device->free(_drawablePointersMemory);
		_drawableBuffer = nullptr;
		_drawableBufferMemory = nullptr;
		_drawableStagingBuffer = nullptr;
		_drawableStagingMemory = nullptr;
		_drawIndirectBuffer = nullptr;
		_drawIndirectMemory = nullptr;
		_drawablePointersBuffer = nullptr;
		_drawablePointersMemory = nullptr;

		// drawable buffer
		_drawableBuffer =
			_device->createBuffer(
				vk::BufferCreateInfo(
					vk::BufferCreateFlags(),      // flags
					_drawableBufferSize,          // size
					vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst |  // usage
						vk::BufferUsageFlagBits::eShaderDeviceAddress,
					vk::SharingMode::eExclusive,  // sharingMode
					0,                            // queueFamilyIndexCount
					nullptr                       // pQueueFamilyIndices
				)
			);
		tie(_drawableBufferMemory, ignore) =
			allocatePointerAccessMemory(_drawableBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
		_device->bindBufferMemory(
			_drawableBuffer,  // buffer
			_drawableBufferMemory,  // memory
			0  // memoryOffset
		);
		_drawableBufferAddress =
			_device->getBufferDeviceAddress(
				vk::BufferDeviceAddressInfo(
					_drawableBuffer  // buffer
				)
			);

		// drawable staging buffer
		_drawableStagingBuffer =
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
		tie(_drawableStagingMemory, ignore) =
			allocateMemory(
				_drawableStagingBuffer,  // buffer
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCached  // requiredFlags
			);
		_device->bindBufferMemory(
			_drawableStagingBuffer,  // buffer
			_drawableStagingMemory,  // memory
			0  // memoryOffset
		);
		_drawableStagingData = reinterpret_cast<DrawableGpuData*>(_device->mapMemory(_drawableStagingMemory, 0, _drawableBufferSize));

		// indirect buffer
		_drawIndirectBuffer =
			_device->createBuffer(
				vk::BufferCreateInfo(
					vk::BufferCreateFlags(),      // flags
					drawIndirectBufferSize,       // size
					vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eStorageBuffer |  // usage
						vk::BufferUsageFlagBits::eShaderDeviceAddress,
					vk::SharingMode::eExclusive,  // sharingMode
					0,                            // queueFamilyIndexCount
					nullptr                       // pQueueFamilyIndices
				)
			);
		tie(_drawIndirectMemory, ignore) =
			allocatePointerAccessMemory(_drawIndirectBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
		_device->bindBufferMemory(
			_drawIndirectBuffer,  // buffer
			_drawIndirectMemory,  // memory
			0  // memoryOffset
		);
		_drawIndirectBufferAddress =
			_device->getBufferDeviceAddress(
				vk::BufferDeviceAddressInfo(
					_drawIndirectBuffer  // buffer
				)
			);

		// payload buffer
		_drawablePointersBuffer =
			_device->createBuffer(
				vk::BufferCreateInfo(
					vk::BufferCreateFlags(),      // flags
					drawablePointersBufferSize,   // size
					vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,  // usage
					vk::SharingMode::eExclusive,  // sharingMode
					0,                            // queueFamilyIndexCount
					nullptr                       // pQueueFamilyIndices
				)
			);
		tie(_drawablePointersMemory, ignore) =
			allocatePointerAccessMemory(_drawablePointersBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
		_device->bindBufferMemory(
			_drawablePointersBuffer,  // buffer
			_drawablePointersMemory,  // memory
			0  // memoryOffset
		);
		_drawablePointersBufferAddress =
			_device->getBufferDeviceAddress(
				vk::BufferDeviceAddressInfo(
					_drawablePointersBuffer  // buffer
				)
			);
	}

	return numDrawables;
}


void Renderer::recordDrawableProcessing(vk::CommandBuffer commandBuffer,size_t numDrawables)
{
	if(numDrawables == 0)
	{
		// write two gpu timestamps
		// as replacement for two timestamps skipped because of the immediate return from this function
		if(_collectFrameInfo) {
			_device->cmdWriteTimestamp(
				commandBuffer,  // commandBuffer
				vk::PipelineStageFlagBits::eTransfer,  // pipelineStage
				_frameInfoTimestampPool,  // queryPool
				_inProgressFrameInfo.numTimestamps++  // query
			);
			_device->cmdWriteTimestamp(
				commandBuffer,  // commandBuffer
				vk::PipelineStageFlagBits::eTransfer,  // pipelineStage
				_frameInfoTimestampPool,  // queryPool
				_inProgressFrameInfo.numTimestamps++  // query
			);
		}

		return;
	}

	// fill Drawable buffer with content
#if 0 // cache flushing is performed by vkQueueSubmit()
	_device->flushMappedMemoryRanges(
		1,  // memoryRangeCount
		array{  // pMemoryRanges
			vk::MappedMemoryRange(
				_drawableStagingMemory,  // memory
				0,  // offset
				_drawableBufferSize  // size
			),
		}.data()
	);
#endif
	_device->cmdCopyBuffer(
		commandBuffer,  // commandBuffer
		_drawableStagingBuffer,  // srcBuffer
		_drawableBuffer,  // dstBuffer
		vk::BufferCopy(  // regions
			0,  // srcOffset
			0,  // dstOffset
			numDrawables * sizeof(DrawableGpuData)  // size
		)  // pRegions
	);
	_device->cmdPipelineBarrier(
		commandBuffer,  // commandBuffer
		vk::PipelineStageFlagBits::eTransfer,  // srcStageMask
		vk::PipelineStageFlagBits::eComputeShader,  // dstStageMask
		vk::DependencyFlags(),  // dependencyFlags
		vk::MemoryBarrier(  // memoryBarriers
			vk::AccessFlagBits::eTransferWrite,  // srcAccessMask
			vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite  // dstAccessMask
		),
		nullptr,  // bufferMemoryBarriers
		nullptr  // imageMemoryBarriers
	);

	// write gpu timestamp
	if(_collectFrameInfo) {
		_device->cmdWriteTimestamp(
			commandBuffer,  // commandBuffer
			vk::PipelineStageFlagBits::eTransfer,  // pipelineStage
			_frameInfoTimestampPool,  // queryPool
			_inProgressFrameInfo.numTimestamps++  // query
		);
	}

	// dispatch drawCommand compute pipeline
	vk::Pipeline pipeline = processDrawablesPipeline(_dataStorage.handleLevel());
	_device->cmdBindPipeline(commandBuffer, vk::PipelineBindPoint::eCompute, pipeline);
	_device->cmdPushConstants(
		commandBuffer,  // commandBuffer
		_processDrawablesPipelineLayout,  // pipelineLayout
		vk::ShaderStageFlagBits::eCompute,  // stageFlags
		0,  // offset
		4*sizeof(uint64_t),  // size
		array<uint64_t,4>{  // pValues
			_dataStorage.handleTableDeviceAddress(),  // handleTablePtr
			_drawableBufferAddress,  // drawableListPtr
			_drawIndirectBufferAddress,  // indirectDataPtr
			_drawablePointersBufferAddress,  // drawablePointersBufferPtr
		}.data()
	);
	if(numDrawables <= 32768)
		_device->cmdDispatch(commandBuffer, uint32_t(numDrawables), 1, 1);
	else {
		assert(numDrawables < (32768*32768) && "Limit of 1Gi of Drawables reached.");
		uint32_t y = ((uint32_t(numDrawables)-1) / 32768);
		uint32_t x = ((uint32_t(numDrawables)-1) % 32768) + 1;
		_device->cmdDispatch(commandBuffer, 32768, y, 1);
		_device->cmdDispatchBase(commandBuffer, 0, y, 0, x, 1, 1);
	}

	// write of gpu timestamp
	if(_collectFrameInfo) {
		_device->cmdWriteTimestamp(
			commandBuffer,  // commandBuffer
			vk::PipelineStageFlagBits::eComputeShader,  // pipelineStage
			_frameInfoTimestampPool,  // queryPool
			_inProgressFrameInfo.numTimestamps++  // query
		);
	}

	// barrier before rendering
#if 0 // this is handled by subpass dependency
	_device->cmdPipelineBarrier(
		commandBuffer,  // commandBuffer
		vk::PipelineStageFlagBits::eComputeShader,  // srcStageMask
		vk::PipelineStageFlagBits::eDrawIndirect | vk::PipelineStageFlagBits::eVertexShader |  // dstStageMask
			vk::PipelineStageFlagBits::eFragmentShader,
		vk::DependencyFlags(),  // dependencyFlags
		vk::MemoryBarrier(  // memoryBarriers
			vk::AccessFlagBits::eShaderWrite,  // srcAccessMask
			vk::AccessFlagBits::eIndirectCommandRead | vk::AccessFlagBits::eShaderRead  // dstAccessMask
		),
		nullptr,  // bufferMemoryBarriers
		nullptr  // imageMemoryBarriers
	);
#endif
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

	// execute all StateSets
	size_t drawableCounter = 0;
	if(_collectFrameInfo == false)
		stateSetRoot.recordToCommandBuffer(commandBuffer, vk::PipelineLayout(), drawableCounter);
	else {
		_inProgressFrameInfo.cpuRecordStateSetsBegin = getCpuTimestamp();
		stateSetRoot.recordToCommandBuffer(commandBuffer, vk::PipelineLayout(), drawableCounter);
		_inProgressFrameInfo.cpuRecordStateSetsEnd = getCpuTimestamp();
	}
	assert(drawableCounter <= _drawableBufferSize/sizeof(DrawableGpuData) && "Buffer overflow. This should not happen.");

	// end render pass
	_device->cmdEndRenderPass(commandBuffer);
}


void Renderer::endRecording(vk::CommandBuffer commandBuffer)
{
	// schedule write of gpu timestamp
	if(collectFrameInfo()) {
		_device->cmdWriteTimestamp(
			commandBuffer,  // commandBuffer
			vk::PipelineStageFlagBits::eBottomOfPipe,  // pipelineStage
			_frameInfoTimestampPool,  // queryPool
			_inProgressFrameInfo.numTimestamps++  // query
		);
	}

	// finish recording of command buffer
	_device->endCommandBuffer(commandBuffer);
}


void Renderer::endFrame()
{
	// signal end of frame to _imageStorage
	_imageStorage.endFrame();

	if(collectFrameInfo()) {

		// write cpu timestamp
		assert(_inProgressFrameInfo.frameNumber == _frameNumber && "Do not start new frame until the recording of previous one is finished.");
		assert(_inProgressFrameInfo.numTimestamps <= FrameInfo::gpuTimestampPoolSize && "Too many timestamps written.");
		_inProgressFrameInfo.cpuEndFrame = getCpuTimestamp();

		// initialize FrameInfo to invalid frame
		_completedFrameInfo = {};
		_completedFrameInfo.frameNumber = ~size_t(0);

	}
}


vk::DeviceMemory Renderer::allocateMemoryTypeNoThrow(size_t size, uint32_t memoryTypeIndex) noexcept
{
	vk::DeviceMemory m;
	vk::MemoryAllocateInfo allocateInfo(
		size,  // allocationSize
		memoryTypeIndex  // memoryTypeIndex
	);
	VkResult r =
		_device->vkAllocateMemory(
			_device->handle(),  // device
			reinterpret_cast<VkMemoryAllocateInfo*>(&allocateInfo),  // pAllocateInfo
			nullptr,  // pAllocator
			reinterpret_cast<VkDeviceMemory*>(&m)  // pMemory
		);
	if(r == VK_SUCCESS)
		return m;
	else
		return nullptr;
}


tuple<vk::DeviceMemory, uint32_t> Renderer::allocateMemory(size_t size, uint32_t memoryTypeBits, vk::MemoryPropertyFlags requiredFlags)
{
	vk::DeviceMemory m;
	vk::MemoryAllocateInfo allocateInfo(
		size,  // allocationSize
		0      // memoryTypeIndex
	);
	for(uint32_t i=0, c=_memoryProperties.memoryTypeCount; i<c; i++)
		if(memoryTypeBits & (1<<i))
			if((_memoryProperties.memoryTypes[i].propertyFlags & requiredFlags) == requiredFlags) {
				allocateInfo.memoryTypeIndex = i;
				VkResult r =
					_device->vkAllocateMemory(
						_device->handle(),  // device
						reinterpret_cast<VkMemoryAllocateInfo*>(&allocateInfo),  // pAllocateInfo
						nullptr,  // pAllocator
						reinterpret_cast<VkDeviceMemory*>(&m)  // pMemory
					);
				if(r == VK_SUCCESS)
					return {m, i};
			}
	throw std::runtime_error("No suitable memory type found for the buffer.");
}


tuple<vk::DeviceMemory, uint32_t> Renderer::allocatePointerAccessMemory(size_t size, uint32_t memoryTypeBits, vk::MemoryPropertyFlags requiredFlags)
{
	vk::DeviceMemory m;
	vk::MemoryAllocateFlagsInfo flagsInfo(
		vk::MemoryAllocateFlagBits::eDeviceAddress,  // flags
		0  // deviceMask
	);
	vk::MemoryAllocateInfo allocateInfo(
		size,  // allocationSize
		0      // memoryTypeIndex
	);
	allocateInfo.setPNext(&flagsInfo);
	for(uint32_t i=0, c=_memoryProperties.memoryTypeCount; i<c; i++)
		if(memoryTypeBits & (1<<i))
			if((_memoryProperties.memoryTypes[i].propertyFlags & requiredFlags) == requiredFlags) {
				allocateInfo.memoryTypeIndex = i;
				VkResult r =
					_device->vkAllocateMemory(
						_device->handle(),  // device
						reinterpret_cast<VkMemoryAllocateInfo*>(&allocateInfo),  // pAllocateInfo
						nullptr,  // pAllocator
						reinterpret_cast<VkDeviceMemory*>(&m)  // pMemory
					);
				if(r == VK_SUCCESS)
					return {m, i};
			}
	throw std::runtime_error("No suitable memory type found for the buffer.");
}


tuple<vk::DeviceMemory, uint32_t> Renderer::allocatePointerAccessMemoryNoThrow(size_t size, uint32_t memoryTypeBits, vk::MemoryPropertyFlags requiredFlags) noexcept
{
	vk::DeviceMemory m;
	vk::MemoryAllocateFlagsInfo flagsInfo(
		vk::MemoryAllocateFlagBits::eDeviceAddress,  // flags
		0  // deviceMask
	);
	vk::MemoryAllocateInfo allocateInfo(
		size,  // allocationSize
		0      // memoryTypeIndex
	);
	allocateInfo.setPNext(&flagsInfo);
	for(uint32_t i=0, c=_memoryProperties.memoryTypeCount; i<c; i++)
		if(memoryTypeBits & (1<<i))
			if((_memoryProperties.memoryTypes[i].propertyFlags & requiredFlags) == requiredFlags) {
				allocateInfo.memoryTypeIndex = i;
				VkResult r =
					_device->vkAllocateMemory(
						_device->handle(),  // device
						reinterpret_cast<VkMemoryAllocateInfo*>(&allocateInfo),  // pAllocateInfo
						nullptr,  // pAllocator
						reinterpret_cast<VkDeviceMemory*>(&m)  // pMemory
					);
				if(r == VK_SUCCESS)
					return {m, i};
			}

	// no suitable memory type found
	return {nullptr, ~uint32_t(0)};
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
	auto [transferResources1, numBytes1] = _dataStorage.recordUploads(_uploadingCommandBuffer);
	_currentFrameUploadBytes += numBytes1;
	auto [transferResources2, numBytes2] = _imageStorage.recordUploads(_uploadingCommandBuffer);
	_currentFrameUploadBytes += numBytes2;

	// end recording
	_device->endCommandBuffer(_uploadingCommandBuffer);

	// if empty, ignore the transfer
	if(numBytes1+numBytes2 == 0)
		return;

	// submit command buffer
	_device->queueSubmit(
		_graphicsQueue,  // queue
		vk::SubmitInfo(  // submits (vk::ArrayProxy)
			0,nullptr,nullptr,          // waitSemaphoreCount,pWaitSemaphores,pWaitDstStageMask
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

	// dispose transfer resources
	// (it must be done only after the transfer is completed)
	transferResources1.release();
	transferResources2.release();
}


void Renderer::setCollectFrameInfo(bool on, bool useCalibratedTimestamps)
{
#if _WIN32
	setCollectFrameInfo(on, useCalibratedTimestamps, vk::TimeDomainEXT::eQueryPerformanceCounter);
#else
	setCollectFrameInfo(on, useCalibratedTimestamps, vk::TimeDomainEXT::eClockMonotonicRaw);
#endif
}


/**
 *  Turns the collection of frame info on or off, while setting sime collection parameters.
 *
 *  The function shall not be called between beginFrame() and endFrame().
 */
void Renderer::setCollectFrameInfo(bool on, bool useCalibratedTimestamps, vk::TimeDomainEXT timestampHostTimeDomain)
{
	_collectFrameInfo = on;
	_useCalibratedTimestamps = useCalibratedTimestamps;
	_timestampHostTimeDomain = timestampHostTimeDomain;
}


const FrameInfo& Renderer::getFrameInfo()
{
	if(!_inProgressFrameInfo.beingCollected)
		return _completedFrameInfo;

	// query for results
	array<uint64_t, FrameInfo::gpuTimestampPoolSize> timestamps;
	vk::Result r =
		_device->getQueryPoolResults(
			_frameInfoTimestampPool,      // queryPool
			0,                            // firstQuery
			_inProgressFrameInfo.numTimestamps,  // queryCount
			FrameInfo::gpuTimestampPoolSize*sizeof(uint64_t),  // dataSize
			timestamps.data(),            // pData
			sizeof(uint64_t),             // stride
			vk::QueryResultFlagBits::e64  // flags
		);

	// if not ready, return invalid frame info
	if(r == vk::Result::eNotReady)
		return _completedFrameInfo;

	_inProgressFrameInfo.beingCollected = false;

	// if success, append the result in l
	// and go to the next _inProgressStats item
	if(r == vk::Result::eSuccess) {
		if(_inProgressFrameInfo.numTimestamps == 4) {
			_inProgressFrameInfo.gpuBeginExecution = timestamps[0];
			_inProgressFrameInfo.gpuAfterTransfersAndBeforeDrawableProcessing = timestamps[1];
			_inProgressFrameInfo.gpuAfterDrawableProcessingAndBeforeRendering = timestamps[2];
			_inProgressFrameInfo.gpuEndExecution = timestamps[3];
		}
		else
			throw LogicError("Renderer::getFrameInfo(): Four timestamps are expected to be recorded "
			                 "during frame rendering and they were not.");

		_completedFrameInfo = _inProgressFrameInfo;
	}

	return _completedFrameInfo;
}


#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN  // exclude rarely-used services inclusion by windows.h; this speeds up compilation and avoids some compilation problems
# include <windows.h>  // we include windows.h only at the end of file to avoid compilation problems; windows.h define MemoryBarrier, near, far and many other problematic macros
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
	if(_useCalibratedTimestamps) {
		uint64_t ts;
		uint64_t maxDeviation;
		_device->getCalibratedTimestampsEXT(
			1,  // timestampCount
			array{ vk::CalibratedTimestampInfoEXT(_timestampHostTimeDomain) }.data(),  // pTimestampInfos
			&ts,  // pTimestamps
			&maxDeviation  // pMaxDeviation
		);
		return ts;
	}
	else {
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
}


uint64_t Renderer::getGpuTimestamp() const
{
	if(_useCalibratedTimestamps) {
		uint64_t ts;
		uint64_t maxDeviation;
		_device->getCalibratedTimestampsEXT(
			1,  // timestampCount
			array{  // pTimestampInfos
				vk::CalibratedTimestampInfoEXT(vk::TimeDomainEXT::eDevice),
			}.data(),
			&ts,  // pTimestamps
			&maxDeviation  // pMaxDeviation
		);
		return ts;
	}
	else {

		// submit command buffer
		_device->queueSubmit(
			_graphicsQueue,  // queue
			vk::SubmitInfo(  // submits (vk::ArrayProxy)
				0, nullptr, nullptr,              // waitSemaphoreCount,pWaitSemaphores,pWaitDstStageMask
				1, &_readTimestampCommandBuffer,  // commandBufferCount,pCommandBuffers
				0, nullptr                        // signalSemaphoreCount,pSignalSemaphores
			),
			_fence  // fence
		);

		// wait for work to complete
		vk::Result r = _device->waitForFences(
			_fence,        // fences (vk::ArrayProxy)
			VK_TRUE,       // waitAll
			uint64_t(3e9)  // timeout (3s)
		);
		_device->resetFences(_fence);
		if(r != vk::Result::eSuccess) {
			if(r == vk::Result::eTimeout)
				throw std::runtime_error("GPU timeout. Task is probably hanging.");
			throw std::runtime_error("vk::Device::waitForFences() returned strange success code.");	 // error codes are already handled by throw inside waitForFences()
		}

		// read timestamp 
		uint64_t ts;
		r = _device->getQueryPoolResults(
			_readTimestampQueryPool,  // queryPool
			0,                    // firstQuery
			1,  // queryCount
			1*sizeof(uint64_t),  // dataSize
			&ts,    // pData
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
		return ts;
	}
}


tuple<uint64_t, uint64_t> Renderer::getCpuAndGpuTimestamps() const
{
	if(_useCalibratedTimestamps) {
		array<uint64_t,2> ts;
		uint64_t maxDeviation;
		_device->getCalibratedTimestampsEXT(
			2,  // timestampCount
			array{  // pTimestampInfos
				vk::CalibratedTimestampInfoEXT(_timestampHostTimeDomain),
				vk::CalibratedTimestampInfoEXT(vk::TimeDomainEXT::eDevice),
			}.data(),
			ts.data(),  // pTimestamps
			&maxDeviation  // pMaxDeviation
		);
		return make_tuple(ts[0], ts[1]);
	}
	else {
		uint64_t gpuTS = getGpuTimestamp();
		uint64_t cpuTS = getCpuTimestamp();
		return make_tuple(cpuTS, gpuTS);
	}
}
