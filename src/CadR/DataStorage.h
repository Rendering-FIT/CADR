#pragma once

#include <CadR/DataAllocation.h>
#include <CadR/DataMemory.h>
#include <CadR/HandleTable.h>
#include <list>
#include <vector>

namespace CadR {

class Renderer;
class StagingMemory;


/** \brief DataStorage class provides GPU data allocation and storage functionality.
 *
 *  The data are stored in DataMemory objects. These are allocated on demand.
 *
 *  CAD applications often cannot determine the amount of required memory for DataStorage
 *  in advance. Traditional solution to reallocate the memory might be a performance problem
 *  for large memory blocks because the allocation of large memory blocks might be costly
 *  (even milliseconds). Thus, we allocate the memory in smaller blocks using DataMemory class
 *  and suballocate it for all allocation requests.
 *
 *  \sa DataMemory, DataAllocation
 */
class CADR_EXPORT DataStorage {
protected:

	Renderer* _renderer;  ///< Rendering context associated with the DataStorage.
	std::vector<DataMemory*> _dataMemoryList;
	DataMemory* _firstAllocMemory = nullptr;
	DataMemory* _secondAllocMemory = nullptr;
	DataMemory _zeroSizeDataMemory = DataMemory(*this, nullptr);
	DataAllocationRecord _zeroSizeAllocationRecord = DataAllocationRecord{ 0, 0, &_zeroSizeDataMemory, nullptr, nullptr, size_t(-2) };
	struct {
		std::list<StagingMemory> inUse64KiBList;
		std::list<StagingMemory> available64KiBList;
		std::list<StagingMemory> inUse2MiBList;
		std::list<StagingMemory> available2MiBList;
		std::list<StagingMemory> inUse32MiBList;
		std::list<StagingMemory> available32MiBList;
		std::list<StagingMemory> inUseSuperSizeList;
		std::list<StagingMemory> availableSuperSizeList;
	} _stagingMemoryRegister;
	size_t _stagingDataSizeHint = 0;
	using TransferInProgressList = std::list<std::vector<std::tuple<StagingMemory*,uint64_t>>>;
	using TransferId = TransferInProgressList::iterator;
	struct TransferRecord {
		TransferId transferId;
		DataStorage* dataStorage;
		TransferRecord(TransferId transferId_, DataStorage* dataStorage_) : transferId(transferId_), dataStorage(dataStorage_)  {}
		void release()  { if(dataStorage==nullptr) return; dataStorage->uploadDone(transferId); dataStorage = nullptr; }
		~TransferRecord()  { if(dataStorage==nullptr) return; dataStorage->uploadDone(transferId); }
		TransferRecord(const TransferRecord&) = delete;
		TransferRecord(TransferRecord&& other) : transferId(other.transferId), dataStorage(other.dataStorage)  { other.dataStorage=nullptr; }
	};
	TransferInProgressList _transferInProgressList;

	CadR::HandleTable _handleTable;

	DataAllocationRecord* allocInternal(size_t numBytes);
	void* stagingAlloc(DataAllocationRecord* allocationRecord, uint64_t offsetInBuffer);

	friend DataAllocation;
	friend DataMemory;
	friend StagingData;

public:

	// construction and destruction
	DataStorage(Renderer& r);
	~DataStorage() noexcept;
	void destroy() noexcept;

	// deleted constructors and operators
	DataStorage(const DataStorage&) = delete;
	DataStorage(DataStorage&&) = delete;
	DataStorage& operator=(const DataStorage&) = delete;
	DataStorage& operator=(DataStorage&&) = delete;

	// getters
	std::vector<DataMemory*>& dataMemoryList();  ///< Returns DataMemory list. It is generally better to use standard Geometry's alloc/realloc/free methods than trying to modify the list directly.
	const std::vector<DataMemory*>& dataMemoryList() const;  ///< Returns DataMemory list.
	Renderer& renderer() const;
	size_t stagingDataSizeHint() const;

	// methods
	DataAllocationRecord* alloc(size_t numBytes);
	DataAllocationRecord* realloc(DataAllocationRecord* allocationRecord, size_t numBytes);
	DataAllocationRecord* zeroSizeAllocationRecord() noexcept;
	void free(DataAllocationRecord* a) noexcept;
	void cancelAllAllocations();
	std::tuple<TransferRecord,size_t> recordUpload(vk::CommandBuffer commandBuffer);
	static void uploadDone(TransferRecord& tr);
	void uploadDone(TransferId id) noexcept;
	void setStagingDataSizeHint(size_t size);

	// handle table
	uint64_t createHandle();
	void destroyHandle(uint64_t handle) noexcept;
	void setHandle(uint64_t handle, uint64_t addr);
	unsigned handleLevel() const;
	uint64_t handleTableDeviceAddress() const;

};


}


// inline and template methods
namespace CadR {

inline std::vector<DataMemory*>& DataStorage::dataMemoryList()  { return _dataMemoryList; }
inline const std::vector<DataMemory*>& DataStorage::dataMemoryList() const  { return _dataMemoryList; }
inline Renderer& DataStorage::renderer() const  { return *_renderer; }
inline size_t DataStorage::stagingDataSizeHint() const  { return _stagingDataSizeHint; }

inline DataAllocationRecord* DataStorage::zeroSizeAllocationRecord() noexcept  { return &_zeroSizeAllocationRecord; }
inline void DataStorage::setStagingDataSizeHint(size_t size)  { _stagingDataSizeHint = size; }
inline void DataStorage::uploadDone(TransferRecord& tr)  { if(tr.dataStorage==nullptr) return; tr.dataStorage->uploadDone(tr.transferId); tr.dataStorage=nullptr; }
inline uint64_t DataStorage::createHandle()  { return _handleTable.create(); }
inline void DataStorage::destroyHandle(uint64_t handle) noexcept  { _handleTable.destroy(handle); }
inline void DataStorage::setHandle(uint64_t handle, uint64_t addr)  { _handleTable.set(handle, addr); }
inline unsigned DataStorage::handleLevel() const  { return _handleTable.handleLevel(); }
inline uint64_t DataStorage::handleTableDeviceAddress() const  { return _handleTable.rootTableDeviceAddress(); }

// functions moved here from DataAllocation.h to avoid circular include dependency
inline HandlelessAllocation::HandlelessAllocation(DataStorage& storage) noexcept  { _record = storage.zeroSizeAllocationRecord(); }
inline HandlelessAllocation::HandlelessAllocation(HandlelessAllocation&& other) noexcept : _record(other._record)  { other._record->recordPointer=&this->_record; other._record=_record->dataMemory->dataStorage().zeroSizeAllocationRecord(); }
inline DataAllocation::DataAllocation(DataStorage& storage, noHandle_t) noexcept : _record(storage.zeroSizeAllocationRecord()), _handle(0)  {}
inline DataAllocation::DataAllocation(DataAllocation&& other) noexcept : _record(other._record), _handle(other._handle)  { _record->recordPointer=&this->_record; other._record=_record->dataMemory->dataStorage().zeroSizeAllocationRecord(); other._handle=0; }
inline HandlelessAllocation& HandlelessAllocation::operator=(HandlelessAllocation&& rhs) noexcept  { free(); _record=rhs._record; rhs._record->recordPointer=&this->_record; rhs._record=_record->dataMemory->dataStorage().zeroSizeAllocationRecord(); return *this; }
inline Renderer& HandlelessAllocation::renderer() const  { return _record->dataMemory->dataStorage().renderer(); }
inline Renderer& DataAllocation::renderer() const  { return _record->dataMemory->dataStorage().renderer(); }
inline void HandlelessAllocation::free() noexcept { if(_record->size==0) return; auto zeroRecord=_record->dataMemory->dataStorage().zeroSizeAllocationRecord(); DataMemory::free(_record); _record=zeroRecord; }
inline void DataAllocation::free() noexcept { if(_record->size==0) return; auto zeroRecord=_record->dataMemory->dataStorage().zeroSizeAllocationRecord(); DataMemory::free(_record); _record=zeroRecord; }
inline void DataAllocation::init(DataStorage& storage)  { if(_record->size!=0) DataMemory::free(_record); _record=storage.zeroSizeAllocationRecord(); }

}

// include inline DataStorage functions defined in Renderer.h (they are defined there because they depend on CadR::Renderer class)
#include <CadR/Renderer.h>
