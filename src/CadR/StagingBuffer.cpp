#include <CadR/StagingBuffer.h>
#include <CadR/Renderer.h>
#include <CadR/VulkanDevice.h>

using namespace CadR;


void StagingBuffer::init()
{
	// handle zero-sized buffer (Vulkan does not like it)
	if(_size==0) {
		_data=nullptr;
		return;
	}

	// create buffer
	VulkanDevice* device=_renderer->device();
	_stgBuffer=
		device->createBuffer(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),      // flags
				_size,                        // size
				vk::BufferUsageFlagBits::eTransferSrc,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr                       // pQueueFamilyIndices
			)
		);
	_stgMemory=
		_renderer->allocateMemory(_stgBuffer,
		                          vk::MemoryPropertyFlagBits::eHostVisible |
		                          vk::MemoryPropertyFlagBits::eHostCoherent |
		                          vk::MemoryPropertyFlagBits::eHostCached);
	device->bindBufferMemory(
		_stgBuffer,  // buffer
		_stgMemory,  // memory
		0            // memoryOffset
	);

	// map buffer
	_data=reinterpret_cast<uint8_t*>(device->mapMemory(_stgMemory,0,_size));
}


void StagingBuffer::cleanUp()
{
	if(_stgBuffer||_stgMemory) {

		// destroy _stgBuffer
		VulkanDevice* device=_renderer->device();
		device->destroy(_stgBuffer);
		_stgBuffer=nullptr;

		// free _stgMemory (this unmaps it as well)
		device->freeMemory(_stgMemory);
		_stgMemory=nullptr;
		_data=nullptr;

	}
}


StagingBuffer::StagingBuffer(StagingBuffer&& sb) noexcept
	: _renderer(sb._renderer)
	, _stgBuffer(sb._stgBuffer)
	, _stgMemory(sb._stgMemory)
	, _data(sb._data)
	, _size(sb._size)
	, _dstBuffer(sb._dstBuffer)
	, _dstOffset(sb._dstOffset)
{
	sb._stgBuffer=nullptr;
	sb._stgMemory=nullptr;
	sb._data=nullptr;
}


StagingBuffer& StagingBuffer::operator=(StagingBuffer&& rhs) noexcept
{
	// release any allocated resources
	cleanUp();

	_renderer=rhs._renderer;
	_stgBuffer=rhs._stgBuffer;
	_stgMemory=rhs._stgMemory;
	_data=rhs._data;
	_size=rhs._size;
	_dstBuffer=rhs._dstBuffer;
	_dstOffset=rhs._dstOffset;
	rhs._stgBuffer=nullptr;
	rhs._stgMemory=nullptr;
	rhs._data=nullptr;
	return *this;
}


void StagingBuffer::reset(vk::Buffer dstBuffer,size_t dstOffset,size_t size,Renderer* renderer)
{
	// release any allocated resources
	cleanUp();

	// set new content
	_renderer=renderer;
	_size=size;
	_dstBuffer=dstBuffer;
	_dstOffset=dstOffset;

	// allocate new resources
	init();
}


void StagingBuffer::submit()
{
	// handle zero-sized buffer (Vulkan does not like it)
	if(!_stgBuffer)
		return;

	// double submission
	if(_data==nullptr)
		throw std::runtime_error("StagingBuffer::submit() called on already submitted buffer.");

	// unmap memory
	_renderer->device()->unmapMemory(_stgMemory);
	_data=nullptr;

	// schedule copy operation
	// (This will transfer _stgBuffer and _stgMemory ownership to _renderer.
	// They will be released after copy operation is completed.)
	_renderer->scheduleCopyOperation(*this);
}


void StagingBuffer::scheduleCopy(vk::CommandBuffer cb)
{
	_renderer->device()->cmdCopyBuffer(cb,_stgBuffer,_dstBuffer,vk::BufferCopy(0,_dstOffset,_size));
}
