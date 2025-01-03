#ifndef CADR_STAGING_MEMORY_HEADER
# define CADR_STAGING_MEMORY_HEADER

# include <vulkan/vulkan.hpp>
# include <boost/intrusive/list.hpp>

namespace CadR {

class DataMemory;
class ImageStorage;
class StagingBuffer;
class StagingManager;


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
	union {
		int64_t _dataMemoryAddrToStagingAddr;  // used by StagingData and DataMemory
		size_t _numBytesAllocated;  // used by StagingBuffer
	};
	uint64_t _referenceCounter = 0;
	StagingManager* _stagingManager;
	vk::Buffer _buffer;
	vk::DeviceMemory _memory;
	uint64_t _bufferStartAddress;  //< Host address of the start of the buffer as seen from the CPU. 
	uint64_t _bufferEndAddress;  //< Host address past the last byte of the buffer as seen from the CPU.
	StagingMemory(StagingManager& stagingManager);
	void ref() noexcept;
	void unref() noexcept;
	friend DataMemory;
	friend ImageStorage;
	friend StagingBuffer;
public:
	boost::intrusive::list_member_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink>
	> _stagingMemoryListHook;  ///< List hook of StagingManager::*List.
public:

	StagingMemory(StagingManager& stagingManager, size_t size);
	~StagingMemory();

	// getters
	template<typename T = void> T* data();
	template<typename T = void> T* data(size_t offset);
	size_t size() const;
	vk::Buffer buffer() const;
	vk::DeviceMemory memory() const;

	// low-level allocation functions
	bool addrRangeOverruns(uint64_t dataMemoryAddr, size_t size) const noexcept;
	bool addrRangeNotOverruns(uint64_t dataMemoryAddr, size_t size) const noexcept;

};


}

#endif


// inline method
#if !defined(CADR_STAGING_MEMORY_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_STAGING_MEMORY_INLINE_FUNCTIONS
# define CADR_NO_INLINE_FUNCTIONS
# include <CadR/StagingManager.h>
# undef CADR_NO_INLINE_FUNCTIONS
namespace CadR {

inline StagingMemory::StagingMemory(StagingManager& stagingManager) : _stagingManager(&stagingManager)  {}
inline void StagingMemory::ref() noexcept  { _referenceCounter++; }
inline void StagingMemory::unref() noexcept  { _referenceCounter--; if(_referenceCounter==0) _stagingManager->freeOrRecycleStagingMemory(*this); }
template<typename T> inline T* StagingMemory::data()  { return reinterpret_cast<T*>(_bufferStartAddress); }
template<typename T> inline T* StagingMemory::data(size_t offset)  { return reinterpret_cast<T*>(_bufferStartAddress+offset); }
inline size_t StagingMemory::size() const  { return _bufferEndAddress - _bufferStartAddress; }
inline vk::Buffer StagingMemory::buffer() const  { return _buffer; }
inline vk::DeviceMemory StagingMemory::memory() const  { return _memory; }
inline bool StagingMemory::addrRangeOverruns(uint64_t dataMemoryAddr, size_t size) const noexcept  { uint64_t stagingAddr = dataMemoryAddr + _dataMemoryAddrToStagingAddr; return stagingAddr + size > _bufferEndAddress; }
inline bool StagingMemory::addrRangeNotOverruns(uint64_t dataMemoryAddr, size_t size) const noexcept  { uint64_t stagingAddr = dataMemoryAddr + _dataMemoryAddrToStagingAddr; return stagingAddr + size <= _bufferEndAddress; }

}
#endif
