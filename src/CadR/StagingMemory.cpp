#include <CadR/StagingMemory.h>
#include <CadR/DataStorage.h>
#include <CadR/Renderer.h>
#include <CadR/VulkanDevice.h>

using namespace std;
using namespace CadR;



StagingMemory::~StagingMemory()
{
	// destroy buffers
	VulkanDevice* device = _dataStorage->renderer()->device();
	device->destroy(_buffer);

	// free memory
	// (this will unmap memory)
	device->freeMemory(_memory);
}


StagingMemory::StagingMemory(DataStorage& dataStorage, size_t size)
	: StagingMemory(dataStorage)  // this ensures the destructor will be executed if this constructor throws
{
	// handle zero-sized buffer
	if(size == 0) {
		_bufferStartAddress = 0;
		_bufferEndAddress = 0;
		_usedBlock2StartAddress = 0;
		_usedBlock2EndAddress = 0;
		_usedBlock1StartAddress = 0;
		_usedBlock1EndAddress = 0;
		return;
	}

	// create _buffer
	Renderer* renderer = dataStorage.renderer();
	VulkanDevice* device = renderer->device();
	_buffer =
		device->createBuffer(
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
	_memory = renderer->allocateMemory(_buffer,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCached);

	// bind memory
	device->bindBufferMemory(
		_buffer,  // buffer
		_memory,  // memory
		0   // memoryOffset
	);

	// get buffer address
	_bufferStartAddress =
		uint64_t(
			device->mapMemory(
				_memory,  // memory
				0,  // offset
				size,  // size
				vk::MemoryMapFlags{}  // flags
			)
		);
	_bufferEndAddress = _bufferStartAddress + size;
	_usedBlock2StartAddress = _bufferStartAddress;
	_usedBlock2EndAddress = _bufferStartAddress;
	_usedBlock1StartAddress = _bufferStartAddress;
	_usedBlock1EndAddress = _bufferStartAddress;
}


StagingDataAllocation* StagingMemory::alloc(DataAllocation* a)
{
	StagingDataAllocation* s = allocInternal(a->size(), a, this);
	a->_stagingDataAllocation = s;
	return s;
}


void StagingMemory::free(StagingDataAllocation* s)
{
	if(!s)
		return;
	if(s->_owner)
		s->_owner->_stagingDataAllocation = nullptr;
	s->_submittedListHook.unlink();
	s->_stagingMemory->freeInternal(s);
}
