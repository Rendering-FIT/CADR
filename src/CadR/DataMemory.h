#pragma once

#include <CadR/CircularAllocationMemory.h>
#define CADR_NO_DATASTORAGE_INCLUDE // this is to break circular dependency; we will include DataStorage.h later in this header
#include <CadR/DataAllocation.h>
#undef CADR_NO_DATASTORAGE_INCLUDE
#include <vulkan/vulkan.hpp>

namespace CadR {

class DataStorage;
class Renderer;
class StagingMemory;


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
class CADR_EXPORT DataMemory : public CircularAllocationMemory<DataAllocationRecord, 200> {
protected:

	DataStorage* _dataStorage;  ///< DataStorage owning this DataMemory.
	vk::Buffer _buffer;
	vk::DeviceMemory _memory;
	std::vector<StagingMemory*> _stagingMemoryList;

	DataMemory(DataStorage& dataStorage);
	friend DataStorage;
	friend StagingMemory;

public:

	// construction and destruction
	static DataMemory* tryCreate(DataStorage& dataStorage, size_t size);  ///< It attempts to create DataMemory. If failure occurs during buffer or memory allocation, it does not throw but returns null.
	DataMemory(DataStorage& dataStorage, size_t size);  ///< Allocates DataMemory, including underlying Vulkan buffer and memory. In the case of failure, exception is thrown. In such case, all the resources including DataMemory object itself are correctly released.
	DataMemory(DataStorage& dataStorage, vk::Buffer buffer, vk::DeviceMemory memory, size_t size);  ///< Allocates DataMemory object while using buffer and memory provided by parameters. The buffer and memory must be valid non-null handles which were already bound together by vkBindBufferMemory() or equivalent call.
	DataMemory(DataStorage& dataStorage, nullptr_t);  ///< Allocates DataMemory object initializing buffer and memory to null and sets size to zero.
	~DataMemory();  ///< Destructor.

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
	vk::DeviceAddress deviceAddress() const;
	size_t usedBytes() const;

	// low-level allocation functions
	// (mostly for internal use)
	DataAllocationRecord* alloc(size_t numBytes);
	static void free(DataAllocationRecord* a) noexcept;
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
inline vk::DeviceAddress DataMemory::deviceAddress() const  { return _bufferStartAddress; }
inline size_t DataMemory::usedBytes() const  { return _usedBytes; }
inline DataAllocationRecord* DataMemory::alloc(size_t numBytes)  { return allocInternal(numBytes, this); }
inline void DataMemory::free(DataAllocationRecord* a) noexcept  { a->dataMemory->freeInternal(a); }

// functions moved here from DataAllocation.h to avoid circular include dependency
inline vk::Buffer HandlelessAllocation::buffer() const  { return _record->dataMemory->buffer(); }
inline size_t HandlelessAllocation::offset() const  { return _record->deviceAddress - _record->dataMemory->deviceAddress(); }
inline DataStorage& HandlelessAllocation::dataStorage() const  { return _record->dataMemory->dataStorage(); }
inline vk::Buffer DataAllocation::buffer() const  { return _record->dataMemory->buffer(); }
inline size_t DataAllocation::offset() const  { return _record->deviceAddress - _record->dataMemory->deviceAddress(); }
inline DataStorage& DataAllocation::dataStorage() const  { return _record->dataMemory->dataStorage(); }

}
