#include <CadR/DataMemory.h>
#include <CadR/DataAllocation.h>
#include <CadR/DataStorage.h>
#include <CadR/Renderer.h>
#include <CadR/StagingMemory.h>
#include <CadR/VulkanDevice.h>

using namespace std;
using namespace CadR;



DataMemory::~DataMemory()
{
	// release blocked memory markers
	// (they are used to block memory after it is scheduled for transfer and until the transfer is completed)
	if(_firstNotTransferredMarker1)
		releaseMemoryMarker1Chain(_firstNotTransferredMarker1);
	if(_firstNotTransferredMarker2)
		releaseMemoryMarker2Chain(_firstNotTransferredMarker2);

	// release buffer and memory
	if(_buffer) {
		VulkanDevice& device = _dataStorage->renderer().device();
		device.destroy(_buffer);
		device.freeMemory(_memory);
	}
}


DataMemory::DataMemory(DataStorage& dataStorage, size_t size)
	: DataMemory(dataStorage)  // this ensures the destructor will be executed if this constructor throws
{
	// handle zero-sized buffer
	if(size == 0) {
		_block1EndAddress = 0;
		_block1StartAddress = 0;
		_block2EndAddress = 0;
		_block2StartAddress = 0;
		_bufferEndAddress = 0;
		_bufferStartAddress = 0;
		return;
	}

	// create _buffer
	Renderer& renderer = dataStorage.renderer();
	VulkanDevice& device = renderer.device();
	_buffer =
		device.createBuffer(
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
	tie(_memory, ignore) =
		renderer.allocatePointerAccessMemory(_buffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

	// bind memory
	device.bindBufferMemory(
		_buffer,  // buffer
		_memory,  // memory
		0   // memoryOffset
	);

	// get buffer address
	_bufferStartAddress =
		device.getBufferDeviceAddress(
			vk::BufferDeviceAddressInfo(
				_buffer  // buffer
			)
		);

	// initialize pointers
	_bufferEndAddress = _bufferStartAddress + size;
	_block1EndAddress = _bufferStartAddress;
	_block1StartAddress = _bufferStartAddress;
	_block2EndAddress = _bufferStartAddress;
	_block2StartAddress = _bufferStartAddress;
}


DataMemory::DataMemory(DataStorage& dataStorage, vk::Buffer buffer, vk::DeviceMemory memory, size_t size)
	: DataMemory(dataStorage)  // this ensures the destructor will be executed if this constructor throws
{
	assert(((buffer && memory && size) || (!buffer && !memory && size==0)) &&
	       "DataMemory::DataMemory(): The parameters buffer, memory and size must all be either non-zero/non-null or zero/null.");

	// assign resources
	// (do not throw in the code above before these are assigned to avoid leaked handles)
	_buffer = buffer;
	_memory = memory;

	// get buffer address
	Renderer& renderer = dataStorage.renderer();
	VulkanDevice& device = renderer.device();
	_bufferStartAddress =
		device.getBufferDeviceAddress(
			vk::BufferDeviceAddressInfo(
				_buffer  // buffer
			)
		);

	// initialize pointers
	_bufferEndAddress = _bufferStartAddress + size;
	_block1EndAddress = _bufferStartAddress;
	_block1StartAddress = _bufferStartAddress;
	_block2EndAddress = _bufferStartAddress;
	_block2StartAddress = _bufferStartAddress;
}


DataMemory::DataMemory(DataStorage& dataStorage, nullptr_t)
	: _dataStorage(&dataStorage)
{
	_block1EndAddress = 0;
	_block1StartAddress = 0;
	_block2EndAddress = 0;
	_block2StartAddress = 0;
	_bufferEndAddress = 0;
	_bufferStartAddress = 0;
}


DataMemory* DataMemory::tryCreate(DataStorage& dataStorage, size_t size)
{
	Renderer& renderer = dataStorage.renderer();
	VulkanDevice& device = renderer.device();
	vk::Device d = device.get();

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
			device
		);
	if(r != vk::Result::eSuccess)
		return nullptr;

	// allocate _memory
	vk::DeviceMemory m;
	tie(m, ignore) =
		renderer.allocatePointerAccessMemoryNoThrow(b, vk::MemoryPropertyFlagBits::eDeviceLocal);
	if(!m) {
		d.destroyBuffer(b, nullptr, device);
		return nullptr;
	}

	// bind memory
	r =
		static_cast<vk::Result>(
			device.vkBindBufferMemory(
				d,  // device
				b,  // buffer
				m,  // memory
				0   // memoryOffset
			)
		);
	if(r != vk::Result::eSuccess) {
		d.freeMemory(m, nullptr, device);
		d.destroyBuffer(b, nullptr, device);
		return nullptr;
	}

	// create DataMemory
	// (if it throws, it correctly releases b and m)
	try {
		return new DataMemory(dataStorage, b, m, size);
	}
	catch(bad_alloc&) {
		d.freeMemory(m, nullptr, device);
		d.destroyBuffer(b, nullptr, device);
		throw;
	}
}


void DataMemory::cancelAllAllocations()
{
#if 0 // disabled as it is questionable what it should exactly do
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
	destroyLastBlock(*this, _allocationBlockList1, _usedBlock1EndAllocation);
	destroyLastBlock(*this, _allocationBlockList2, _usedBlock2EndAllocation);

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
	destroyRegularBlocks(*this, _allocationBlockList2);
	destroyRegularBlocks(*this, _allocationBlockList1);
#endif
}


struct MarkerAllocationRecord
{
	vk::DeviceAddress deviceAddress;
	size_t size;
	StagingMemory* stagingMemory;
	uint64_t stagingStartAddress;
	uint64_t stagingEndAddress;
	MarkerAllocationRecord* nextRecord;

	void init(vk::DeviceAddress addr)  { deviceAddress = addr; size = 0; stagingMemory = nullptr; stagingStartAddress = 0; stagingEndAddress = 0; nextRecord = nullptr; }
};


DataAllocationRecord* DataMemory::alloc(size_t numBytes)
{
	// propose allocation
	// (it will be confirmed when we call alloc[1|2]Commit(),
	// before commit no resources are really allocated)
	auto [addr, blockNumber] = allocPropose(numBytes);

	// avoid many "if" in this function by abstracting some variables and functions
	DataAllocationRecord* (DataMemory::*allocZeroSize)();
	DataAllocationRecord* lastStagingMarker;
	DataAllocationRecord** lastStagingMarkerVariable;
	StagingMemory* lastStagingMemory;
	StagingMemory** lastStagingMemoryVariable;
	StagingMemory* otherLastStagingMemory;
	DataAllocationRecord* (DataMemory::*allocCommit)(uint64_t, size_t);
	if(blockNumber == 1) {
		allocZeroSize = &DataMemory::alloc1ZeroSize;
		lastStagingMarker = _lastStagingMarker1;
		lastStagingMarkerVariable = &_lastStagingMarker1;
		lastStagingMemory = _lastStagingMemory1;
		lastStagingMemoryVariable = &_lastStagingMemory1;
		otherLastStagingMemory = _lastStagingMemory2;
		allocCommit = &DataMemory::alloc1Commit;
	}
	else if(blockNumber == 2) {
		allocZeroSize = &DataMemory::alloc2ZeroSize;
		lastStagingMarker = _lastStagingMarker2;
		lastStagingMarkerVariable = &_lastStagingMarker2;
		lastStagingMemory = _lastStagingMemory2;
		lastStagingMemoryVariable = &_lastStagingMemory2;
		otherLastStagingMemory = _lastStagingMemory1;
		allocCommit = &DataMemory::alloc2Commit;
	}
	else
		return nullptr;

	// do we have marker already?
	if(lastStagingMarker) {

		// do we have enough space in the current StagingMemory?
		// if no, handle it by reusing exclusive StagingMemory or allocating new StagingMemory
		if(lastStagingMemory->addrRangeOverruns(addr, numBytes)) {

			// get current MarkerAllocationRecord
			MarkerAllocationRecord* marker = reinterpret_cast<MarkerAllocationRecord*>(lastStagingMarker);

			// create new MarkerAllocationRecord
			lastStagingMarker = (this->*allocZeroSize)();  // might throw
			*lastStagingMarkerVariable = lastStagingMarker;
			marker->nextRecord = reinterpret_cast<MarkerAllocationRecord*>(lastStagingMarker);
			marker = reinterpret_cast<MarkerAllocationRecord*>(lastStagingMarker);
			marker->init(addr);

			if(otherLastStagingMemory && otherLastStagingMemory->size() == size())
			{
				// reuse exclusive StagingMemory from otherLastStagingMemory
				lastStagingMemory = otherLastStagingMemory;
				*lastStagingMemoryVariable = lastStagingMemory;
				lastStagingMemory->_referenceCounter++;
				marker->stagingMemory = lastStagingMemory;
				marker->stagingStartAddress = uint64_t(addr) + lastStagingMemory->_dataMemoryAddrToStagingAddr;
			}
			else
			{
				// alloc StagingMemory
				auto[sm, exclusive] =
					_dataStorage->allocStagingMemory(*this, lastStagingMemory, numBytes, _bufferEndAddress - addr);  // might throw
				lastStagingMemory = &sm;
				marker->stagingMemory = lastStagingMemory;
				*lastStagingMemoryVariable = lastStagingMemory;
				lastStagingMemory->_referenceCounter++;
				if(exclusive) {
					lastStagingMemory->_dataMemoryAddrToStagingAddr = lastStagingMemory->_bufferStartAddress - _bufferStartAddress;
					marker->stagingStartAddress = uint64_t(addr) + lastStagingMemory->_dataMemoryAddrToStagingAddr;
				}
				else {
					lastStagingMemory->_dataMemoryAddrToStagingAddr = lastStagingMemory->_bufferStartAddress - addr;
					marker->stagingStartAddress = lastStagingMemory->_bufferStartAddress; 
				}
			}
		}
	}
	else {

		// create MarkerAllocationRecord
		lastStagingMarker = (this->*allocZeroSize)();  // might throw
		*lastStagingMarkerVariable = lastStagingMarker;
		if(blockNumber == 1)
			_firstNotTransferredMarker1 = lastStagingMarker;
		else
			_firstNotTransferredMarker2 = lastStagingMarker;
		MarkerAllocationRecord* marker = reinterpret_cast<MarkerAllocationRecord*>(lastStagingMarker);
		marker->init(addr);

		// StagingMemory
		if(lastStagingMemory && lastStagingMemory->addrRangeNotOverruns(addr, numBytes))
		{
			// reuse StagingMemory
			marker->stagingMemory = lastStagingMemory;
			marker->stagingStartAddress = addr + lastStagingMemory->_dataMemoryAddrToStagingAddr;
			lastStagingMemory->_referenceCounter++;
		}
		else
		{
			if(otherLastStagingMemory && otherLastStagingMemory->size() == size())
			{
				// reuse exclusive StagingMemory from otherLastStagingMemory
				lastStagingMemory = otherLastStagingMemory;
				*lastStagingMemoryVariable = lastStagingMemory;
				lastStagingMemory->_referenceCounter++;
				marker->stagingMemory = lastStagingMemory;
				marker->stagingStartAddress = uint64_t(addr) + lastStagingMemory->_dataMemoryAddrToStagingAddr;
			}
			else
			{
				// alloc StagingMemory
				auto[sm, exclusive] =
					_dataStorage->allocStagingMemory(*this, lastStagingMemory, numBytes, _bufferEndAddress - addr);  // might throw
				lastStagingMemory = &sm;
				marker->stagingMemory = lastStagingMemory;
				*lastStagingMemoryVariable = lastStagingMemory;
				lastStagingMemory->_referenceCounter++;
				if(exclusive) {
					lastStagingMemory->_dataMemoryAddrToStagingAddr = lastStagingMemory->_bufferStartAddress - _bufferStartAddress;
					marker->stagingStartAddress = uint64_t(addr) + lastStagingMemory->_dataMemoryAddrToStagingAddr;
				}
				else {
					lastStagingMemory->_dataMemoryAddrToStagingAddr = lastStagingMemory->_bufferStartAddress - addr;
					marker->stagingStartAddress = lastStagingMemory->_bufferStartAddress; 
				}
			}
		}
	}

	// return allocation
	DataAllocationRecord* a = (this->*allocCommit)(addr, numBytes);  // might throw
	uint64_t stagingAddr = addr + lastStagingMemory->_dataMemoryAddrToStagingAddr;
	a->init(addr, numBytes, this, nullptr,
	        reinterpret_cast<void*>(stagingAddr),
	        size_t(-2));
	reinterpret_cast<MarkerAllocationRecord*>(lastStagingMarker)->stagingEndAddress = stagingAddr + numBytes;
	return a;
}


[[nodiscard]] std::tuple<void*,void*,size_t> DataMemory::recordUploads(vk::CommandBuffer commandBuffer)
{
	if(_firstNotTransferredMarker1 == nullptr && _firstNotTransferredMarker2 == nullptr)
		return { nullptr, nullptr, 0 };

	VulkanDevice& device = dataStorage().renderer().device();
	size_t numBytesTransferred = 0;

	auto processBlock =
		[](DataAllocationRecord* firstNotTransferredMarker, size_t& numBytesTransferred, VulkanDevice& device,
		   vk::CommandBuffer commandBuffer, vk::Buffer dstBuffer, uint64_t dstBufferStartAddress)
		{
			MarkerAllocationRecord* marker = reinterpret_cast<MarkerAllocationRecord*>(firstNotTransferredMarker);

			do {
				StagingMemory* sm = marker->stagingMemory;
				uint64_t size = marker->stagingEndAddress - marker->stagingStartAddress;
				device.cmdCopyBuffer(
					commandBuffer,  // commandBuffer
					sm->buffer(),  // srcBuffer
					dstBuffer,  // dstBuffer
					vk::BufferCopy(  // regions
						marker->stagingStartAddress - sm->_bufferStartAddress,  // srcOffset
						marker->deviceAddress - dstBufferStartAddress,  // dstOffset
						size)  // size
				);
				numBytesTransferred += size;

				marker = marker->nextRecord;
			} while(marker != nullptr);
		};

	if(_firstNotTransferredMarker1)
		processBlock(_firstNotTransferredMarker1, numBytesTransferred, device, commandBuffer, _buffer, _bufferStartAddress);

	if(_firstNotTransferredMarker2)
		processBlock(_firstNotTransferredMarker2, numBytesTransferred, device, commandBuffer, _buffer, _bufferStartAddress);

	void* stagingMarkers1 = _firstNotTransferredMarker1;
	void* stagingMarkers2 = _firstNotTransferredMarker2;
	_lastStagingMarker1 = nullptr;
	_lastStagingMarker2 = nullptr;
	_firstNotTransferredMarker1 = nullptr;
	_firstNotTransferredMarker2 = nullptr;

	return { stagingMarkers1, stagingMarkers2, numBytesTransferred };
}


void DataMemory::uploadDone(void* stagingMarkers1, void* stagingMarkers2) noexcept
{
	if(stagingMarkers1)
		releaseMemoryMarker1Chain(reinterpret_cast<DataAllocationRecord*>(stagingMarkers1));
	if(stagingMarkers2)
		releaseMemoryMarker2Chain(reinterpret_cast<DataAllocationRecord*>(stagingMarkers2));
}


void DataMemory::releaseMemoryMarker1Chain(DataAllocationRecord* marker)
{
	do {

		// decrement reference counter
		StagingMemory* sm = reinterpret_cast<MarkerAllocationRecord*>(marker)->stagingMemory;
		if(sm) {
			sm->_referenceCounter--;
			if(sm->_referenceCounter == 0) {
				if(_lastStagingMemory1 == sm)
					_lastStagingMemory1 = nullptr;
				if(_lastStagingMemory2 == sm)
					_lastStagingMemory2 = nullptr;
				_dataStorage->freeOrRecycleStagingMemory(*sm);
			}
		}

		// free marker
		MarkerAllocationRecord* nextRecord = reinterpret_cast<MarkerAllocationRecord*>(marker)->nextRecord;
		freeInternal1ZeroSize(marker);

		// move to the next marker
		marker = reinterpret_cast<DataAllocationRecord*>(nextRecord);

	} while(marker != nullptr);
}


void DataMemory::releaseMemoryMarker2Chain(DataAllocationRecord* marker)
{
	do {

		// decrement reference counter
		StagingMemory* sm = reinterpret_cast<MarkerAllocationRecord*>(marker)->stagingMemory;
		if(sm) {
			sm->_referenceCounter--;
			if(sm->_referenceCounter == 0) {
				if(_lastStagingMemory1 == sm)
					_lastStagingMemory1 = nullptr;
				if(_lastStagingMemory2 == sm)
					_lastStagingMemory2 = nullptr;
				_dataStorage->freeOrRecycleStagingMemory(*sm);
			}
		}

		// free marker
		MarkerAllocationRecord* nextRecord = reinterpret_cast<MarkerAllocationRecord*>(marker)->nextRecord;
		freeInternal2ZeroSize(marker);

		// move to the next marker
		marker = reinterpret_cast<DataAllocationRecord*>(nextRecord);

	} while(marker != nullptr);
}
