#include <CadR/StagingMemory.h>
#include <CadR/DataStorage.h>
#include <CadR/Exceptions.h>
#include <CadR/Renderer.h>
#include <CadR/StagingManager.h>
#include <CadR/VulkanDevice.h>

using namespace std;
using namespace CadR;



StagingMemory::~StagingMemory()
{
	// destroy buffers
	VulkanDevice& device = _stagingManager->renderer().device();
	device.destroy(_buffer);

	// free memory
	// (this will unmap memory)
	device.freeMemory(_memory);
}


StagingMemory::StagingMemory(StagingManager& stagingManager, size_t size)
	: StagingMemory(stagingManager)  // this ensures the destructor will be executed if this constructor throws
{
	// handle zero-sized buffer
	if(size == 0) {
		_bufferStartAddress = 0;
		_bufferEndAddress = 0;
		return;
	}

	// create _buffer
	Renderer& renderer = stagingManager.renderer();
	VulkanDevice& device = renderer.device();
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
	tie(_memory, ignore) =
		renderer.allocateMemory(_buffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCached);

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
}
