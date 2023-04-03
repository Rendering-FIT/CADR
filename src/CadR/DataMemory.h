#pragma once

#include <CadR/CircularAllocationMemory.h>
#include <CadR/DataAllocation.h>
#include <vulkan/vulkan.hpp>

namespace CadR {

class DataStorage;


/** \brief
 *  DataMemory class provides GPU memory functionality
 *  for allocating and storing user data.
 *  DataMemory class is usually used together with DataAllocation
 *  and DataStorage classes.
 * 
 *  All the data of DataMemory are stored in single vk::Buffer.
 *  The buffer is allocated during DataMemory construction
 *  and cannot change its size afterwards. If you need more
 *  space, allocate another DataMemory object. Optionally,
 *  you might copy the content of the old DataMemory object
 *  into new one and delete the old object.
 *
 *  \sa DataStorage, DataAllocation
 */
class CADR_EXPORT DataMemory : public CircularAllocationMemory<DataAllocation, 200> {
public:
	using MoveCallback = DataAllocation::MoveCallback;
protected:

	DataStorage* _dataStorage;  ///< DataStorage owning this DataMemory.

	vk::Buffer _buffer;
	vk::DeviceMemory _memory;

	DataMemory(DataStorage& dataStorage);

public:

	// construction and destruction
	static DataMemory* tryCreate(DataStorage& dataStorage, size_t size);
	DataMemory(DataStorage& dataStorage, size_t size);
	DataMemory(DataStorage& dataStorage, vk::Buffer buffer, vk::DeviceMemory memory, size_t size);
	~DataMemory();

	// deleted constructors and operators
	DataMemory() = delete;
	DataMemory(const DataMemory&) = delete;
	DataMemory& operator=(const DataMemory&) = delete;
	DataMemory(DataMemory&&) = delete;
	DataMemory& operator=(DataMemory&&) = delete;

	// getters
	DataStorage& dataStorage() const;
	size_t size() const;
	vk::Buffer buffer() const;
	vk::DeviceMemory memory() const;
	vk::DeviceAddress bufferDeviceAddress() const;
	size_t usedBytes() const;

	// low-level allocation functions
	DataAllocation* alloc(size_t numBytes, MoveCallback& moveCallback, void* moveCallbackUserData);
	static void free(DataAllocation* a);
	void cancelAllAllocations();

};


}


// inline methods
namespace CadR {

inline DataMemory::DataMemory(DataStorage& dataStorage) : _dataStorage(&dataStorage)  {}
inline DataStorage& DataMemory::dataStorage() const  { return *_dataStorage; }
inline size_t DataMemory::size() const  { return _bufferEndAddress - _bufferStartAddress; }
inline vk::Buffer DataMemory::buffer() const  { return _buffer; }
inline vk::DeviceMemory DataMemory::memory() const  { return _memory; }
inline vk::DeviceAddress DataMemory::bufferDeviceAddress() const  { return _bufferStartAddress; }
inline size_t DataMemory::usedBytes() const  { return _usedBytes; }
inline DataAllocation* DataMemory::alloc(size_t numBytes, MoveCallback& moveCallback, void* moveCallbackUserData)  { return allocInternal(numBytes, numBytes, this, moveCallback, moveCallbackUserData); }
inline void DataMemory::free(DataAllocation* a)  { if(a) a->dataMemory().freeInternal(a); }

// functions moved from DataAllocation.h to avoid circular include dependency
// (allowed by include CadR/DataAllocation.h on the top of this file)
inline void DataAllocation::free()  { if(_stagingDataAllocation) _stagingDataAllocation->free(); DataMemory::free(this); }
inline vk::Buffer DataAllocation::buffer() const  { return _dataMemory->buffer(); }
inline size_t DataAllocation::offset() const  { return _deviceAddress - _dataMemory->bufferDeviceAddress(); }
inline DataStorage& DataAllocation::dataStorage() const  { return _dataMemory->dataStorage(); }

}
