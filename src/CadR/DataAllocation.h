#ifndef CADR_DATA_ALLOCATION_HEADER
# define CADR_DATA_ALLOCATION_HEADER

# ifndef CADR_NO_INLINE_FUNCTIONS
#  define CADR_NO_INLINE_FUNCTIONS
#  include <CadR/StagingData.h>
#  undef CADR_NO_INLINE_FUNCTIONS
# else
#  include <CadR/StagingData.h>
# endif
# include <vulkan/vulkan.hpp>

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

	void init(vk::DeviceAddress addr, size_t size, DataMemory* m, DataAllocationRecord** recordPointer,
	          void* stagingData, size_t stagingFrameNumber);
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

#endif


// inline methods
#if !defined(CADR_DATA_ALLOCATION_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_DATA_ALLOCATION_INLINE_FUNCTIONS
# define CADR_NO_INLINE_FUNCTIONS
# include <CadR/DataStorage.h>
# undef CADR_NO_INLINE_FUNCTIONS
namespace CadR {

inline void DataAllocationRecord::init(vk::DeviceAddress addr, size_t size, DataMemory* m, DataAllocationRecord** recordPointer, void* stagingData, size_t stagingFrameNumber)  { deviceAddress = addr; this->size = size; dataMemory = m; this->recordPointer = recordPointer; this->stagingData = stagingData; this->stagingFrameNumber = stagingFrameNumber; }
inline DataAllocation::DataAllocation(nullptr_t) noexcept : _record(&DataAllocationRecord::nullRecord), _handle(0)  {}
inline DataAllocation::DataAllocation(DataStorage& storage) : _record(storage.zeroSizeAllocationRecord()), _handle(storage.createHandle())  {}  // this might throw in DataStorage::createHandle(), but _record points to zero size record that does not need to be freed
inline DataAllocation::DataAllocation(DataStorage& storage, noHandle_t) noexcept : _record(storage.zeroSizeAllocationRecord()), _handle(0)  {}
inline DataAllocation::DataAllocation(DataAllocation&& other) noexcept : _record(other._record), _handle(other._handle)  { _record->recordPointer=&this->_record; other._record=_record->dataMemory->dataStorage().zeroSizeAllocationRecord(); other._handle=0; }
inline DataAllocation::~DataAllocation() noexcept  { free(); if(_handle!=0) dataStorage().destroyHandle(_handle); }
inline HandlelessAllocation::HandlelessAllocation(nullptr_t) noexcept : _record(&DataAllocationRecord::nullRecord)  {}
inline HandlelessAllocation::HandlelessAllocation(DataStorage& storage) noexcept  { _record = storage.zeroSizeAllocationRecord(); }
inline HandlelessAllocation::HandlelessAllocation(HandlelessAllocation&& other) noexcept : _record(other._record)  { other._record->recordPointer=&this->_record; other._record=_record->dataMemory->dataStorage().zeroSizeAllocationRecord(); }
inline HandlelessAllocation::~HandlelessAllocation() noexcept  { free(); }

inline DataAllocation& DataAllocation::operator=(DataAllocation&& rhs) noexcept  { free(); if(_handle!=0) dataStorage().destroyHandle(_handle); _record=rhs._record; _handle = rhs._handle; rhs._record->recordPointer=&this->_record; rhs._record=_record->dataMemory->dataStorage().zeroSizeAllocationRecord(); rhs._handle=0; return *this; }
inline HandlelessAllocation& HandlelessAllocation::operator=(HandlelessAllocation&& rhs) noexcept  { free(); _record=rhs._record; rhs._record->recordPointer=&this->_record; rhs._record=_record->dataMemory->dataStorage().zeroSizeAllocationRecord(); return *this; }

inline void DataAllocation::init(DataStorage& storage)  { if(_record->size!=0) DataMemory::free(_record); _record=storage.zeroSizeAllocationRecord(); }
inline void HandlelessAllocation::init(DataStorage& storage)  { if(_record->size!=0) DataMemory::free(_record); _record=storage.zeroSizeAllocationRecord(); }
inline void DataAllocation::free() noexcept { if(_record->size==0) return; auto zeroRecord=_record->dataMemory->dataStorage().zeroSizeAllocationRecord(); DataMemory::free(_record); _record=zeroRecord; }
inline void HandlelessAllocation::free() noexcept { if(_record->size==0) return; auto zeroRecord=_record->dataMemory->dataStorage().zeroSizeAllocationRecord(); DataMemory::free(_record); _record=zeroRecord; }
inline StagingData DataAllocation::createStagingData()  { return alloc(); }
inline StagingData DataAllocation::createStagingData(size_t size)  { return alloc(size); }
inline StagingData HandlelessAllocation::createStagingData()  { return alloc(); }
inline StagingData HandlelessAllocation::createStagingData(size_t size)  { return alloc(size); }
inline uint64_t DataAllocation::createHandle(DataStorage& storage)  { if(_handle==0) _handle=storage.createHandle(); return _handle; }
inline void DataAllocation::destroyHandle() noexcept  { if(_handle==0) return; dataStorage().destroyHandle(_handle); _handle=0; }

inline uint64_t DataAllocation::handle() const  { return _handle; }
inline vk::DeviceAddress DataAllocation::deviceAddress() const  { return _record->deviceAddress; }
inline vk::DeviceAddress HandlelessAllocation::deviceAddress() const  { return _record->deviceAddress; }
inline size_t DataAllocation::size() const  { return _record->size; }
inline size_t HandlelessAllocation::size() const  { return _record->size; }
inline vk::Buffer DataAllocation::buffer() const  { return _record->dataMemory->buffer(); }
inline vk::Buffer HandlelessAllocation::buffer() const  { return _record->dataMemory->buffer(); }
inline size_t DataAllocation::offset() const  { return _record->deviceAddress - _record->dataMemory->deviceAddress(); }
inline size_t HandlelessAllocation::offset() const  { return _record->deviceAddress - _record->dataMemory->deviceAddress(); }
inline DataMemory& DataAllocation::dataMemory() const  { return *_record->dataMemory; }
inline DataMemory& HandlelessAllocation::dataMemory() const  { return *_record->dataMemory; }
inline DataStorage& DataAllocation::dataStorage() const  { return _record->dataMemory->dataStorage(); }
inline DataStorage& HandlelessAllocation::dataStorage() const  { return _record->dataMemory->dataStorage(); }
inline Renderer& DataAllocation::renderer() const  { return _record->dataMemory->dataStorage().renderer(); }
inline Renderer& HandlelessAllocation::renderer() const  { return _record->dataMemory->dataStorage().renderer(); }

}
#endif
