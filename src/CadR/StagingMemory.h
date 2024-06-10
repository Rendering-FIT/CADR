#pragma once

#include <vulkan/vulkan.hpp>

namespace CadR {

class DataMemory;
class Renderer;


/** \brief StagingMemory represents memory used to upload data to
 *  DataAllocation and HandlelessAllocation objects.
 *
 *  The memory serves as staging buffer for possibly large group
 *  of DataAllocations and HandlelessAllocations that
 *  are updated together in one transfer operation, thus
 *  performing fast updates of large amount of objects.
 *
 *  Each DataAllocation and HandlelessAllocation update is
 *  performed through StagingData. The data update is expected to
 *  be completed before call to Renderer::submitCopyOperations()
 *  or next frame rendering start.
 *
 *  \sa StagingData, DataAllocation, HandlelessAllocation, DataStorage, DataMemory
 */
class StagingMemory {
protected:
	DataMemory* _attachedDataMemory = 0;
	uint64_t _offsetIntoDataMemory;
	Renderer* _renderer;
	vk::Buffer _buffer;
	vk::DeviceMemory _memory;
	size_t _bufferSize;
	uint64_t _bufferStartAddress;
	uint64_t _bufferEndAddress;
	uint64_t _blockedRangeStartAddress;
	uint64_t _blockedRangeEndAddress;
	uint64_t _allocatedRangeStartAddress;
	uint64_t _allocatedRangeEndAddress;
	StagingMemory(Renderer& renderer);
public:

	StagingMemory(Renderer& renderer, size_t size);
	~StagingMemory();

	void attach(DataMemory& m, uint64_t offset);
	void detach() noexcept;
	bool canDetach() const;

	// getters
	DataMemory* dataMemory() const;
	uint64_t offsetIntoDataMemory() const;
	size_t size() const;
	vk::Buffer buffer() const;
	vk::DeviceMemory memory() const;
	size_t usedBytes() const;

	// low-level allocation functions
	uint64_t alloc(uint64_t offsetInBuffer, uint64_t size) noexcept;
	void cancelAllAllocations();
	std::tuple<uint64_t,size_t> recordUpload(vk::CommandBuffer);
	void uploadDone(uint64_t id) noexcept;

};


}


// inline methods
namespace CadR {

inline StagingMemory::StagingMemory(Renderer& renderer) : _renderer(&renderer)  {}
inline bool StagingMemory::canDetach() const  { return _allocatedRangeEndAddress == _blockedRangeStartAddress; }
inline DataMemory* StagingMemory::dataMemory() const  { return _attachedDataMemory; }
inline uint64_t StagingMemory::offsetIntoDataMemory() const  { return _offsetIntoDataMemory; }
inline size_t StagingMemory::size() const  { return _bufferSize; }
inline vk::Buffer StagingMemory::buffer() const  { return _buffer; }
inline vk::DeviceMemory StagingMemory::memory() const  { return _memory; }
inline size_t StagingMemory::usedBytes() const  { return (_allocatedRangeEndAddress > _blockedRangeStartAddress) ? _allocatedRangeEndAddress - _blockedRangeStartAddress : _blockedRangeEndAddress - _allocatedRangeStartAddress; }
inline uint64_t StagingMemory::alloc(uint64_t offsetInBuffer, uint64_t size) noexcept  { uint64_t addr = _bufferStartAddress + offsetInBuffer - _offsetIntoDataMemory; uint64_t addrEnd = addr + size; if(addrEnd > _bufferEndAddress) return 0; _allocatedRangeEndAddress = addrEnd; return addr; }

}
