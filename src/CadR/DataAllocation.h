#pragma once

#define CADR_NO_DATAALLOCATION_INCLUDE
#include <CadR/StagingData.h>
#undef CADR_NO_DATAALLOCATION_INCLUDE
#include <vulkan/vulkan.hpp>

namespace CadR {

class DataAllocation;
class DataMemory;
class DataStorage;
class Renderer;


struct CADR_EXPORT DataAllocationRecord
{
	vk::DeviceAddress deviceAddress;
	size_t size;
	DataMemory* dataMemory;
	DataAllocationRecord** recordPointer;
	void* stagingData;
	size_t stagingFrameNumber;

	void init(vk::DeviceAddress addr, size_t size, DataMemory* m);
	static DataAllocationRecord nullRecord;
};


/** \brief DataAllocation represents single piece of allocated memory.
 *
 *  The allocated memory is suballocated from DataMemory objects and managed using DataStorage.
 *
 *  \sa DataMemory, DataStorage
 */
class CADR_EXPORT DataAllocation {
protected:
	DataAllocationRecord* _record;
	uint64_t _handle;
public:

	// construction and destruction
	//using noHandle_t = struct NoHandle*;
	enum class noHandle_t : int;
	static constexpr const noHandle_t noHandle = noHandle_t(0);
	DataAllocation(nullptr_t) noexcept;
	DataAllocation(DataStorage& storage);
	DataAllocation(DataStorage& storage, noHandle_t) noexcept;
	DataAllocation(DataAllocation&&) noexcept;  ///< Move constructor.
	DataAllocation(const DataAllocation&) = delete;  ///< No copy constructor.
	~DataAllocation() noexcept;  ///< Destructor.

	// operators
	DataAllocation& operator=(DataAllocation&&) noexcept;  ///< Move assignment operator.
	DataAllocation& operator=(const DataAllocation&) = delete;  ///< No copy assignment.

	// alloc and free
	void init(DataStorage& storage);
	StagingData alloc(size_t size);
	StagingData alloc();
	void free() noexcept;
	StagingData createStagingData();
	StagingData createStagingData(size_t size);
	uint64_t createHandle(DataStorage& storage);
	void destroyHandle() noexcept;

	// getters
	uint64_t handle() const;
	vk::DeviceAddress deviceAddress() const;
	size_t size() const;
	vk::Buffer buffer() const;
	size_t offset() const;
	DataMemory& dataMemory() const;
	DataStorage& dataStorage() const;
	Renderer& renderer() const;

	// setters and data update
	void upload(const void* ptr, size_t numBytes);

};


class CADR_EXPORT HandlelessAllocation {
protected:
	DataAllocationRecord* _record;
public:

	// construction and destruction
	HandlelessAllocation(nullptr_t) noexcept;
	HandlelessAllocation(DataStorage& storage) noexcept;
	HandlelessAllocation(HandlelessAllocation&&) noexcept;  ///< Move constructor.
	HandlelessAllocation(const HandlelessAllocation&) = delete;  ///< No copy constructor.
	~HandlelessAllocation() noexcept;  ///< Destructor.

	// operators
	HandlelessAllocation& operator=(HandlelessAllocation&&) noexcept;  ///< Move assignment operator.
	HandlelessAllocation& operator=(const HandlelessAllocation&) = delete;  ///< No copy assignment.

	// alloc and free
	void init(DataStorage& storage);
	StagingData alloc(size_t size);
	StagingData alloc();
	void free() noexcept;
	StagingData createStagingData();
	StagingData createStagingData(size_t size);

	// getters
	vk::DeviceAddress deviceAddress() const;
	size_t size() const;
	vk::Buffer buffer() const;
	size_t offset() const;
	DataMemory& dataMemory() const;
	DataStorage& dataStorage() const;
	Renderer& renderer() const;

	// setters and data update
	void upload(const void* ptr, size_t numBytes);

};


}


// inline methods
namespace CadR {

inline void DataAllocationRecord::init(vk::DeviceAddress addr, size_t size, DataMemory* m)  { deviceAddress = addr; this->size = size; dataMemory = m; }
inline HandlelessAllocation::HandlelessAllocation(nullptr_t) noexcept : _record(&DataAllocationRecord::nullRecord)  {}
inline HandlelessAllocation::~HandlelessAllocation() noexcept  { free(); }
inline DataAllocation::DataAllocation(nullptr_t) noexcept : _record(&DataAllocationRecord::nullRecord), _handle(0)  {}

inline StagingData DataAllocation::createStagingData()  { return alloc(); }
inline StagingData DataAllocation::createStagingData(size_t size)  { return alloc(size); }
inline StagingData HandlelessAllocation::createStagingData()  { return alloc(); }
inline StagingData HandlelessAllocation::createStagingData(size_t size)  { return alloc(size); }

inline vk::DeviceAddress HandlelessAllocation::deviceAddress() const  { return _record->deviceAddress; }
inline size_t HandlelessAllocation::size() const  { return _record->size; }
inline DataMemory& HandlelessAllocation::dataMemory() const  { return *_record->dataMemory; }
inline uint64_t DataAllocation::handle() const  { return _handle; }
inline vk::DeviceAddress DataAllocation::deviceAddress() const  { return _record->deviceAddress; }
inline size_t DataAllocation::size() const  { return _record->size; }
inline DataMemory& DataAllocation::dataMemory() const  { return *_record->dataMemory; }

// functions moved here to avoid circular include dependency
template<typename T> inline T* StagingData::data()  { return reinterpret_cast<T*>(_record->stagingData); }
inline size_t StagingData::sizeInBytes() const  { return _record->size; }

}


// include inline DataAllocation functions defined in DataStorage.h (they are defined there because they depend on CadR::DataStorage class)
#ifndef CADR_NO_DATASTORAGE_INCLUDE
# include <CadR/DataStorage.h>
#endif

// we skipped StagingMemory by CADR_NO_STAGINGMEMORY_INCLUDE, so let's include it here
// otherwise some inline methods of StagingData might be missing
#include <CadR/StagingMemory.h>
