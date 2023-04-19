#pragma once

#include <CadR/StagingData.h>
#include <vulkan/vulkan.hpp>

namespace CadR {

class DataMemory;
class DataStorage;
class StagingDataAllocation;


/** \brief DataAllocation represents single piece of allocated memory.
 *
 *  The data are stored in DataMemory objects and managed using DataStorage.
 *
 *  \sa DataMemory, DataAllocation
 */
class CADR_EXPORT DataAllocation {
public:
	using MoveCallback = void (*)(DataAllocation* oldAlloc, DataAllocation* newAlloc, void* userData);
protected:
	vk::DeviceAddress _deviceAddress;
	size_t _size;
	DataMemory* _dataMemory;
	MoveCallback _moveCallback;
	void* _moveCallbackUserData;
	StagingDataAllocation* _stagingDataAllocation;
	friend DataMemory;
	friend DataStorage;
	friend StagingDataAllocation;
	friend class StagingMemory;
public:

	// construction
	static DataAllocation* alloc(DataStorage& storage, size_t size,
	                             MoveCallback moveCallback, void* moveCallbackUserData);
	DataAllocation() = default;
	DataAllocation(vk::DeviceAddress deviceAddress, size_t size, DataMemory* dataMemory,
	               MoveCallback moveCallback, void* moveCallbackUserData);
	void init(vk::DeviceAddress deviceAddress, size_t size, DataMemory* dataMemory,
	          MoveCallback moveCallback, void* moveCallbackUserData);
	void free();

	// deleted constructors and operators
	DataAllocation(const DataAllocation&) = delete;
	DataAllocation(DataAllocation&&) = delete;
	DataAllocation& operator=(const DataAllocation&) = delete;
	DataAllocation& operator=(DataAllocation&&) = delete;

	// getters
	vk::DeviceAddress deviceAddress() const;
	size_t size() const;
	vk::Buffer buffer() const;
	size_t offset() const;
	DataMemory& dataMemory() const;
	DataStorage& dataStorage() const;
	MoveCallback moveCallback() const;
	void* moveCallbackUserData() const;

	// setters and data update
	void setMoveCallback(MoveCallback cb, void* userData);
	void setMoveCallback(MoveCallback cb);
	void setMoveCallbackUserData(void* userData);
	void upload(void* data);
	StagingData createStagingData();
	//void subupload(void* data, size_t offset, size_t size);
	//void* getStagingBuffer(size_t offset, size_t size);

};


}


// inline methods
namespace CadR {

inline DataAllocation::DataAllocation(vk::DeviceAddress deviceAddress, size_t size, DataMemory* dataMemory, MoveCallback moveCallback, void* moveCallbackUserData)  { init(deviceAddress, size, dataMemory, moveCallback, moveCallbackUserData); }
inline void DataAllocation::init(vk::DeviceAddress deviceAddress, size_t size, DataMemory* dataMemory, MoveCallback moveCallback, void* moveCallbackUserData)  { _deviceAddress = deviceAddress; _size = size; _dataMemory = dataMemory; _moveCallback = moveCallback; _moveCallbackUserData = moveCallbackUserData; _stagingDataAllocation = nullptr; }
inline vk::DeviceAddress DataAllocation::deviceAddress() const  { return _deviceAddress; }
inline size_t DataAllocation::size() const  { return _size; }
inline DataMemory& DataAllocation::dataMemory() const  { return *_dataMemory; }
inline DataAllocation::MoveCallback DataAllocation::moveCallback() const  { return _moveCallback; }
inline void* DataAllocation::moveCallbackUserData() const  { return _moveCallbackUserData; }
inline void DataAllocation::setMoveCallback(MoveCallback cb, void* userData)  { _moveCallback = cb; _moveCallbackUserData = userData; }
inline void DataAllocation::setMoveCallback(MoveCallback cb)  { _moveCallback = cb; }
inline void DataAllocation::setMoveCallbackUserData(void* userData)  { _moveCallbackUserData = userData; }

// functions moved here to avoid circular include dependency
inline size_t StagingData::size() const  { return _stagingDataAllocation->_owner->size(); }
inline size_t StagingDataAllocation::size() const  { return _owner->size(); }
inline DataStorage& StagingDataAllocation::dataStorage() const  { return _owner->dataStorage(); }
inline DataMemory& StagingDataAllocation::destinationDataMemory() const  { return _owner->dataMemory(); }
inline size_t StagingDataAllocation::destinationOffset() const  { return _owner->offset(); }

}
