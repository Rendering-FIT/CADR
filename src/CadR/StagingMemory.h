#pragma once

#include <CadR/CircularAllocationMemory.h>
#include <CadR/StagingDataAllocation.h>
#include <vulkan/vulkan.hpp>

namespace CadR {


/** \brief StagingMemory represents memory that is suballocated to StagingData
 *  and StagingDataAllocation objects.
 *
 *  The memory serves as staging buffer. The user updates the staging buffer
 *  and calls StagingData::submit(). The update operation is scheduled
 *  to copy staging data into the data associated with particular DataAllocation.
 *
 *  \sa StagingData, StagingDataAllocation, DataAllocation, DataStorage, DataMemory
 */
class StagingMemory : public CircularAllocationMemory<StagingDataAllocation, 200> {
protected:
	DataStorage* _dataStorage;  ///< DataStorage owning this StagingMemory.
	vk::Buffer _buffer;
	vk::DeviceMemory _memory;
	StagingMemory(DataStorage& dataStorage);
public:

	StagingMemory(DataStorage& dataStorage, size_t size);
	~StagingMemory();

	// getters
	DataStorage& dataStorage() const;
	size_t size() const;
	vk::Buffer buffer() const;
	vk::DeviceMemory memory() const;
	vk::DeviceAddress bufferDeviceAddress() const;
	size_t usedBytes() const;

	// low-level allocation functions
	StagingDataAllocation* alloc(DataAllocation* a);
	static void free(StagingDataAllocation* a);
	void cancelAllAllocations();

};


}


// inline methods
namespace CadR {

inline StagingMemory::StagingMemory(DataStorage& dataStorage) : _dataStorage(&dataStorage)  {}
inline DataStorage& StagingMemory::dataStorage() const  { return *_dataStorage; }
inline size_t StagingMemory::size() const  { return _bufferEndAddress - _bufferStartAddress; }
inline vk::Buffer StagingMemory::buffer() const  { return _buffer; }
inline vk::DeviceMemory StagingMemory::memory() const  { return _memory; }
inline vk::DeviceAddress StagingMemory::bufferDeviceAddress() const  { return _bufferStartAddress; }
inline size_t StagingMemory::usedBytes() const  { return _usedBytes; }

// functions moved here to avoid circular include dependency
inline size_t StagingDataAllocation::stagingOffset() const  { return size_t(reinterpret_cast<uint8_t*>(_data) - _stagingMemory->bufferDeviceAddress()); }
inline void StagingDataAllocation::free()  { StagingMemory::free(this); }

}
