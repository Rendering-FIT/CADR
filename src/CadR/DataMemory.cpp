#include <CadR/DataMemory.h>
#include <CadR/DataStorage.h>
#include <CadR/Renderer.h>
#include <CadR/VulkanDevice.h>

using namespace std;
using namespace CadR;

DataAllocation DataMemory::_zeroSizeAllocation(0, 0, nullptr, nullptr, nullptr);



DataMemory::~DataMemory()
{
	// destroy buffers
	VulkanDevice* device = _dataStorage->renderer()->device();
	device->destroy(_buffer);

	// free memory
	device->freeMemory(_memory);
}


DataMemory::DataMemory(DataStorage& dataStorage, size_t size)
	: DataMemory(dataStorage)  // this ensures the destructor will be executed if this constructor throws
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
				vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress |  // usage
					vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
				vk::SharingMode::eExclusive,  // sharingMode
				0,  // queueFamilyIndexCount
				nullptr  // pQueueFamilyIndices
			)
		);

	// allocate _memory
	_memory = renderer->allocatePointerAccessMemory(_buffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

	// bind memory
	device->bindBufferMemory(
		_buffer,  // buffer
		_memory,  // memory
		0   // memoryOffset
	);

	// get buffer address
	_bufferStartAddress =
		device->getBufferDeviceAddress(
			vk::BufferDeviceAddressInfo(
				_buffer  // buffer
			)
		);
	_bufferEndAddress = _bufferStartAddress + size;
	_usedBlock2StartAddress = _bufferStartAddress;
	_usedBlock2EndAddress = _bufferStartAddress;
	_usedBlock1StartAddress = _bufferStartAddress;
	_usedBlock1EndAddress = _bufferStartAddress;
}


DataMemory::DataMemory(DataStorage& dataStorage, vk::Buffer buffer, vk::DeviceMemory memory, size_t size)
	: DataMemory(dataStorage)  // this ensures the destructor will be executed if this constructor throws
{
	assert(((buffer && memory && size) || (!buffer && !memory && size==0)) &&
	       "DataMemory::DataMemory(): The buffer, memory and size parameters must all be either non-zero/non-null or zero/null.");

	// assign resources
	_buffer = buffer;
	_memory = memory;

	// get buffer address
	Renderer* renderer = dataStorage.renderer();
	VulkanDevice* device = renderer->device();
	_bufferStartAddress = 
		device->getBufferDeviceAddress(
			vk::BufferDeviceAddressInfo(
				_buffer  // buffer
			)
		);
	_bufferEndAddress = _bufferStartAddress + size;
	_usedBlock2StartAddress = _bufferStartAddress;
	_usedBlock2EndAddress = _bufferStartAddress;
	_usedBlock1StartAddress = _bufferStartAddress;
	_usedBlock1EndAddress = _bufferStartAddress;
}


DataMemory* DataMemory::tryCreate(DataStorage& dataStorage, size_t size)
{
	Renderer* renderer = dataStorage.renderer();
	VulkanDevice* device = renderer->device();
	assert(device && "CadR::Renderer object is not initialized. It's device is null.");
	vk::Device d = device->get();

	// try create buffer
	vk::Buffer b;
	vk::Result r =
		d.createBuffer(
			&(const vk::BufferCreateInfo&)vk::BufferCreateInfo(
				vk::BufferCreateFlags(),  // flags
				size,  // size
				vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress |  // usage
					vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
				vk::SharingMode::eExclusive,  // sharingMode
				0,  // queueFamilyIndexCount
				nullptr  // pQueueFamilyIndices
			),
			nullptr,
			&b,
			*device
		);
	if(r != vk::Result::eSuccess)
		return nullptr;

	// allocate _memory
	vk::DeviceMemory m = renderer->allocatePointerAccessMemoryNoThrow(b, vk::MemoryPropertyFlagBits::eDeviceLocal);
	if(!m) {
		d.destroyBuffer(b, nullptr, *device);
		return nullptr;
	}

	// bind memory
	r =
		static_cast<vk::Result>(
			device->vkBindBufferMemory(
				d,  // device
				b,  // buffer
				m,  // memory
				0   // memoryOffset
			)
		);
	if(r != vk::Result::eSuccess) {
		d.freeMemory(m, nullptr, *device);
		d.destroyBuffer(b, nullptr, *device);
		return nullptr;
	}

	// create DataMemory
	// (this might throw, theoretically)
	return new DataMemory(dataStorage, b, m, size);
}


void DataMemory::cancelAllAllocations()
{
	auto destroyLastBlock =
		[](DataMemory& m, AllocationBlockList& l, AllocationBlockIterator endAllocation)
		{
			AllocationBlock& b = l.back();
			for(auto it=b.allocations.begin(); it!=b.allocations.end(); it++) {
				if(it == endAllocation)
					break;
				if(it->_deviceAddress == 0)
					continue;
				if(it->_deviceAddress == ~vk::DeviceAddress(0))
					break;
				if(it->_moveCallback)
					it->_moveCallback(&*it, nullptr, it->_moveCallbackUserData);
			}
			m.destroyAllocationBlock(b);
		};
	destroyLastBlock(*this, _allocationBlockList2, _usedBlock2EndAllocation);
	destroyLastBlock(*this, _allocationBlockList1, _usedBlock1EndAllocation);

	auto destroyRegularBlocks =
		[](DataMemory& m, AllocationBlockList& l)
		{
			for(AllocationBlock& b : l) {
				for(auto it=b.allocations.begin(); it!=b.allocations.end(); it++) {
					if(it->_deviceAddress == 0)
						continue;
					if(it->_deviceAddress == ~vk::DeviceAddress(0))
						break;
					if(it->_moveCallback)
						it->_moveCallback(&*it, nullptr, it->_moveCallbackUserData);
				}
				m.destroyAllocationBlock(b);
			}
		};
	destroyRegularBlocks(*this, _allocationBlockList1);
	destroyRegularBlocks(*this, _allocationBlockList2);
}
