#include <CadR/StagingMemory.h>
#include <CadR/DataStorage.h>
#include <CadR/Exceptions.h>
#include <CadR/Renderer.h>
#include <CadR/VulkanDevice.h>

using namespace std;
using namespace CadR;



StagingMemory::~StagingMemory()
{
	// destroy buffers
	VulkanDevice& device = _renderer->device();
	device.destroy(_buffer);

	// free memory
	// (this will unmap memory)
	device.freeMemory(_memory);
}


StagingMemory::StagingMemory(Renderer& renderer, size_t size)
	: StagingMemory(renderer)  // this ensures the destructor will be executed if this constructor throws
{
	// handle zero-sized buffer
	if(size == 0) {
		_bufferStartAddress = 0;
		_bufferEndAddress = 0;
		_bufferSize = 0;
		_blockedRangeStartAddress = 0;
		_blockedRangeEndAddress = 0;
		_allocatedRangeStartAddress = 0;
		_allocatedRangeEndAddress = 0;
		return;
	}

	// create _buffer
	VulkanDevice& device = _renderer->device();
	_buffer =
		device.createBuffer(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),  // flags
				size,  // size
				vk::BufferUsageFlagBits::eTransferSrc,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,  // queueFamilyIndexCount
				nullptr  // pQueueFamilyIndices
			)
		);

	// allocate _memory
	_memory = _renderer->allocateMemory(_buffer,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCached);

	// bind memory
	device.bindBufferMemory(
		_buffer,  // buffer
		_memory,  // memory
		0   // memoryOffset
	);

	// get buffer address
	_bufferStartAddress =
		uint64_t(
			device.mapMemory(
				_memory,  // memory
				0,  // offset
				size,  // size
				vk::MemoryMapFlags{}  // flags
			)
		);
	_bufferEndAddress = _bufferStartAddress + size;
	_bufferSize = size;
	_blockedRangeStartAddress = _bufferStartAddress;
	_blockedRangeEndAddress = _bufferStartAddress;
	_allocatedRangeStartAddress = _bufferStartAddress;
	_allocatedRangeEndAddress = _bufferStartAddress;
}


void StagingMemory::attach(DataMemory& m, uint64_t offset)
{
	assert(_attachedDataMemory == nullptr && "StagingMemory is already attached.");

	_attachedDataMemory = &m;
	_offsetIntoDataMemory = offset;
	_blockedRangeStartAddress = _bufferStartAddress;
	_blockedRangeEndAddress = _bufferStartAddress;
	_allocatedRangeStartAddress = _bufferStartAddress;
	_allocatedRangeEndAddress = _bufferStartAddress;
	m._stagingMemoryList.push_back(this);
}


void StagingMemory::detach() noexcept
{
	assert(_attachedDataMemory != nullptr && "StagingMemory is not attached.");

	if(_attachedDataMemory->_stagingMemoryList.back() == this)
		_attachedDataMemory->_stagingMemoryList.pop_back();
	else {
		auto it = find(_attachedDataMemory->_stagingMemoryList.begin(), _attachedDataMemory->_stagingMemoryList.end(), this);
		assert(it != _attachedDataMemory->_stagingMemoryList.end() && "StagingMemory is not in the list.");
		if(it != _attachedDataMemory->_stagingMemoryList.end())
			_attachedDataMemory->_stagingMemoryList.erase(it);
	}

	_attachedDataMemory = nullptr;
}


tuple<uint64_t,size_t> StagingMemory::recordUpload(vk::CommandBuffer commandBuffer)
{
	assert(_attachedDataMemory && "StagingMemory::upload(): DataMemory must be attached to StagingMemory before calling upload().");

	// record new transfer
	VulkanDevice& device = _renderer->device();
	uint64_t numBytesToTransfer = _allocatedRangeEndAddress - _allocatedRangeStartAddress;
	uint64_t srcOffset = _allocatedRangeStartAddress - _bufferStartAddress;
	uint64_t dstOffset = _offsetIntoDataMemory + srcOffset;
	device.flushMappedMemoryRanges(
		1,  // memoryRangeCount
		array{  // pMemoryRanges
			vk::MappedMemoryRange(
				_memory,  // memory
				0,//_allocatedRangeStartAddress - _bufferStartAddress,  // offset
				VK_WHOLE_SIZE//_drawableBufferSize  // size
			),
		}.data()
	);
	device.cmdCopyBuffer(
		commandBuffer,  // commandBuffer
		_buffer,  // srcBuffer
		_attachedDataMemory->buffer(),  // dstBuffer
		vk::BufferCopy(  // regions
			srcOffset,  // srcOffset
			dstOffset,  // dstOffset
			numBytesToTransfer)  // size
	);

	// update blocked range
	// (blocked range marks place where transfer is in progress)
	_blockedRangeEndAddress = _allocatedRangeEndAddress;

	// return end of blocked range and amount of bytes to transfer
	return { _blockedRangeEndAddress, numBytesToTransfer };
}


void StagingMemory::uploadDone(uint64_t id) noexcept
{
	_blockedRangeStartAddress = id;
}
