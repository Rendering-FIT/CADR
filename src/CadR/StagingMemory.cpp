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
}
