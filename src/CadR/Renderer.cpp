#include <CadR/Renderer.h>
#include <CadR/AttribStorage.h>
#include <CadR/AllocationManagers.h>
#include <CadR/StagingBuffer.h>
#include <CadR/VulkanDevice.h>
#include <CadR/VulkanInstance.h>

using namespace std;
using namespace CadR;

Renderer* Renderer::_instance = nullptr;



Renderer::Renderer(VulkanDevice* device,VulkanInstance* instance,vk::PhysicalDevice physicalDevice,uint32_t graphicsQueueFamily)
	: _device(device)
	, _graphicsQueueFamily(graphicsQueueFamily)
	, _indexAllocationManager(1024,0)  // set capacity to 1024, zero-sized null object (on index 0)
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

	// submitNowCommandBuffer
	// that will be submitted at the end of this function
	_commandPoolTransient=
		(*device)->createCommandPool(
			vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlagBits::eTransient|vk::CommandPoolCreateFlagBits::eResetCommandBuffer,  // flags
				_graphicsQueueFamily  // queueFamilyIndex
			),
			nullptr,*_device
		);
	_uploadingCommandBuffer=
		(*device)->allocateCommandBuffers(
			vk::CommandBufferAllocateInfo(
				_commandPoolTransient,             // commandPool
				vk::CommandBufferLevel::ePrimary,  // level
				1                                  // commandBufferCount
			),
			*_device
		)[0];
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
	assert(_emptyStorage->allocationManager().numIDs()==1 && "Renderer::_emptyStorage is not empty. It is a programmer error to allocate anything there. You probably called Mesh::allocAttribs() without specifying AttribConfig.");

	_uploadingCommandBuffer.end(*_device);
	(*_device)->destroy(_commandPoolTransient,nullptr,*_device);  // no need to destroy commandBuffers as destroying command pool frees all command buffers allocated from the pool
	purgeObjectsToDeleteAfterCopyOperation();

	// index buffer
	(*_device)->destroy(_indexBuffer,nullptr,*_device);
	(*_device)->freeMemory(_indexBufferMemory,nullptr,*_device);

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
