#pragma once

#include <vulkan/vulkan.hpp>
#include <boost/intrusive/list.hpp>

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
	int64_t _dataMemoryAddrToStagingAddr;
	uint64_t _referenceCounter = 0;
	Renderer* _renderer;
	vk::Buffer _buffer;
	vk::DeviceMemory _memory;
	size_t _bufferSize;
	uint64_t _bufferStartAddress;
	uint64_t _bufferEndAddress;
	StagingMemory(Renderer& renderer);
	friend DataMemory;
public:
	boost::intrusive::list_member_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink>
	> _stagingMemoryListHook;  ///< List hook of DataStorage::_stagingMemoryRegister::*List.
public:

	StagingMemory(Renderer& renderer, size_t size);
	~StagingMemory();

	// getters
	size_t size() const;
	vk::Buffer buffer() const;
	vk::DeviceMemory memory() const;

	// low-level allocation functions
	bool addrRangeOverruns(uint64_t dataMemoryAddr, size_t size) const noexcept;
	bool addrRangeNotOverruns(uint64_t dataMemoryAddr, size_t size) const noexcept;

};


}


// inline methods
namespace CadR {

inline StagingMemory::StagingMemory(Renderer& renderer) : _renderer(&renderer)  {}
inline size_t StagingMemory::size() const  { return _bufferSize; }
inline vk::Buffer StagingMemory::buffer() const  { return _buffer; }
inline vk::DeviceMemory StagingMemory::memory() const  { return _memory; }
inline bool StagingMemory::addrRangeOverruns(uint64_t dataMemoryAddr, size_t size) const noexcept  { uint64_t stagingAddr = dataMemoryAddr + _dataMemoryAddrToStagingAddr; return stagingAddr + size > _bufferEndAddress; }
inline bool StagingMemory::addrRangeNotOverruns(uint64_t dataMemoryAddr, size_t size) const noexcept  { uint64_t stagingAddr = dataMemoryAddr + _dataMemoryAddrToStagingAddr; return stagingAddr + size <= _bufferEndAddress; }

}
