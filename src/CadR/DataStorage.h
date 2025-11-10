#ifndef CADR_DATA_STORAGE_HEADER
# define CADR_DATA_STORAGE_HEADER

# ifndef CADR_NO_INLINE_FUNCTIONS
#  define CADR_NO_INLINE_FUNCTIONS
#  include <CadR/DataMemory.h>
#  include <CadR/HandleTable.h>
#  include <CadR/TransferResources.h>
#  undef CADR_NO_INLINE_FUNCTIONS
# else
#  include <CadR/DataMemory.h>
#  include <CadR/HandleTable.h>
#  include <CadR/TransferResources.h>
# endif
# include <vector>

namespace CadR {

class Renderer;
class StagingManager;
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
	StagingManager* _stagingManager;
	size_t _stagingDataSizeHint = 0;

	CadR::HandleTable _handleTable;

	DataAllocationRecord* allocInternal(size_t numBytes);

	std::tuple<StagingMemory&, bool> allocStagingMemory(DataMemory& m,
		StagingMemory* lastStagingMemory, size_t minNumBytes, size_t bytesToMemoryEnd);
	inline void freeOrRecycleStagingMemory(StagingMemory& sm);

	friend DataAllocation;
	friend DataMemory;
	friend StagingData;

public:

	// construction and destruction
	inline DataStorage(Renderer& r) noexcept;
	inline ~DataStorage() noexcept;
	inline void init(StagingManager& stagingManager);
	void cleanUp() noexcept;

	// deleted constructors and operators
	DataStorage(const DataStorage&) = delete;
	DataStorage(DataStorage&&) = delete;
	DataStorage& operator=(const DataStorage&) = delete;
	DataStorage& operator=(DataStorage&&) = delete;

	// getters
	inline std::vector<DataMemory*>& dataMemoryList();  ///< Returns DataMemory list. It is generally better to use standard Geometry's alloc/realloc/free methods than trying to modify the list directly.
	inline const std::vector<DataMemory*>& dataMemoryList() const;  ///< Returns DataMemory list.
	inline Renderer& renderer() const;
	inline size_t stagingDataSizeHint() const;

	// functions
	DataAllocationRecord* alloc(size_t numBytes);
	DataAllocationRecord* realloc(DataAllocationRecord* allocationRecord, size_t numBytes);
	inline DataAllocationRecord* zeroSizeAllocationRecord() noexcept;
	inline void free(DataAllocationRecord* a) noexcept;
	void cancelAllAllocations();

	// data upload
	std::tuple<TransferResources,size_t> recordUploads(vk::CommandBuffer commandBuffer);
	inline void setStagingDataSizeHint(size_t size);

	// handle table
	inline uint64_t createHandle();
	inline void destroyHandle(uint64_t handle) noexcept;
	inline void setHandle(uint64_t handle, uint64_t addr);
	inline unsigned handleLevel() const;
	inline uint64_t handleTableDeviceAddress() const;

};


}

#endif


// inline and template methods
#if !defined(CADR_DATA_STORAGE_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_DATA_STORAGE_INLINE_FUNCTIONS
# define CADR_NO_INLINE_FUNCTIONS
# include <CadR/StagingManager.h>
# undef CADR_NO_INLINE_FUNCTIONS
# include <CadR/DataMemory.h>
# include <CadR/HandleTable.h>
namespace CadR {

inline void DataStorage::freeOrRecycleStagingMemory(StagingMemory& sm)  { _stagingManager->freeOrRecycleStagingMemory(sm); }
inline DataStorage::DataStorage(Renderer& renderer) noexcept  : _renderer(&renderer), _stagingManager(nullptr), _handleTable(*this) {}
inline DataStorage::~DataStorage() noexcept  { cleanUp(); }
inline void DataStorage::init(StagingManager& stagingManager)  { _stagingManager = &stagingManager; }

inline std::vector<DataMemory*>& DataStorage::dataMemoryList()  { return _dataMemoryList; }
inline const std::vector<DataMemory*>& DataStorage::dataMemoryList() const  { return _dataMemoryList; }
inline Renderer& DataStorage::renderer() const  { return *_renderer; }
inline size_t DataStorage::stagingDataSizeHint() const  { return _stagingDataSizeHint; }

inline DataAllocationRecord* DataStorage::zeroSizeAllocationRecord() noexcept  { return &_zeroSizeAllocationRecord; }
inline void DataStorage::free(DataAllocationRecord* a) noexcept  { if(a->size==0) return; DataMemory::free(a); }
inline void DataStorage::setStagingDataSizeHint(size_t size)  { _stagingDataSizeHint = size; }
inline uint64_t DataStorage::createHandle()  { return _handleTable.create(); }
inline void DataStorage::destroyHandle(uint64_t handle) noexcept  { _handleTable.destroy(handle); }
inline void DataStorage::setHandle(uint64_t handle, uint64_t addr)  { _handleTable.set(handle, addr); }
inline unsigned DataStorage::handleLevel() const  { return _handleTable.handleLevel(); }
inline uint64_t DataStorage::handleTableDeviceAddress() const  { return _handleTable.rootTableDeviceAddress(); }

}
#endif
