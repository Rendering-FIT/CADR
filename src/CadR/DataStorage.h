#pragma once

#include <CadR/DataAllocation.h>
#include <CadR/DataMemory.h>
#include <CadR/HandleTable.h>
#include <CadR/StagingMemory.h>
#include <CadR/TransferResources.h>
#include <boost/intrusive/list.hpp>
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
	struct StagingMemoryRegister {
		using StagingMemoryList =
			boost::intrusive::list<
				StagingMemory,
				boost::intrusive::member_hook<
					StagingMemory,
					boost::intrusive::list_member_hook<
						boost::intrusive::link_mode<boost::intrusive::auto_unlink>>,
					&StagingMemory::_stagingMemoryListHook>,
				boost::intrusive::constant_time_size<false>
			>;
		struct Disposer {
			Disposer& operator()(StagingMemory* sm)  { delete sm; return *this; }
		};
		StagingMemoryList smallMemoryInUseList;
		StagingMemoryList smallMemoryAvailableList;
		StagingMemoryList middleMemoryInUseList;
		StagingMemoryList middleMemoryAvailableList;
		StagingMemoryList largeMemoryInUseList;
		StagingMemoryList largeMemoryAvailableList;
		StagingMemoryList superSizeMemoryInUseList;
		StagingMemoryList superSizeMemoryAvailableList;
	} _stagingMemoryRegister;
	static constexpr const size_t _smallMemorySize = 64 << 10;  // 64KiB
	static constexpr const size_t _middleMemorySize = 2 << 20;  // 2MiB
	static constexpr const size_t _largeMemorySize = 32 << 20;  // 32MiB
	size_t _stagingDataSizeHint = 0;
	using TransferList = std::list<TransferResources>;
	TransferList _transferInProgressList;

	CadR::HandleTable _handleTable;

	DataAllocationRecord* allocInternal(size_t numBytes);
	void uploadDone(TransferResourcesReleaser::Id id) noexcept;

	std::tuple<StagingMemory&, bool> allocStagingMemory(DataMemory& m,
		StagingMemory* lastStagingMemory, size_t minNumBytes, size_t bytesToMemoryEnd);
	void freeOrRecycleStagingMemory(StagingMemory& sm);

	friend DataAllocation;
	friend DataMemory;
	friend StagingData;
	friend TransferResourcesReleaser;

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

	// functions
	DataAllocationRecord* alloc(size_t numBytes);
	DataAllocationRecord* realloc(DataAllocationRecord* allocationRecord, size_t numBytes);
	DataAllocationRecord* zeroSizeAllocationRecord() noexcept;
	void free(DataAllocationRecord* a) noexcept;
	void cancelAllAllocations();

	using TransferId = TransferList::iterator;
	std::tuple<TransferResourcesReleaser,size_t> recordUploads(vk::CommandBuffer commandBuffer);
	static void uploadDone(TransferResourcesReleaser& trr);
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
inline void DataStorage::uploadDone(TransferResourcesReleaser& trr)  { trr.release(); }
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

// functions moved here from TransferResources.h
inline void TransferResourcesReleaser::release()  { if(_dataStorage==nullptr) return; _dataStorage->uploadDone(_id); _dataStorage=nullptr; }
inline TransferResourcesReleaser::~TransferResourcesReleaser()  { if(_dataStorage!=nullptr) _dataStorage->uploadDone(_id); }
inline TransferResourcesReleaser& TransferResourcesReleaser::operator=(TransferResourcesReleaser&& other)  { if(_dataStorage!=nullptr) _dataStorage->uploadDone(_id); _id=other._id; _dataStorage=other._dataStorage; other._dataStorage=nullptr; return *this; }

}

// include inline DataStorage functions defined in Renderer.h (they are defined there because they depend on CadR::Renderer class)
#include <CadR/Renderer.h>
