#ifndef CADR_DATA_MEMORY_HEADER
# define CADR_DATA_MEMORY_HEADER

# ifndef CADR_NO_INLINE_FUNCTIONS
#  define CADR_NO_INLINE_FUNCTIONS
#  include <CadR/CircularAllocationMemory.h>
#  include <CadR/DataAllocation.h>
#  undef CADR_NO_INLINE_FUNCTIONS
# else
#  include <CadR/CircularAllocationMemory.h>
#  include <CadR/DataAllocation.h>
# endif
# include <vulkan/vulkan.hpp>

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
	StagingMemory* _lastStagingMemory1 = nullptr;
	StagingMemory* _lastStagingMemory2 = nullptr;
	DataAllocationRecord* _lastStagingMarker1 = nullptr;
	DataAllocationRecord* _lastStagingMarker2 = nullptr;
	DataAllocationRecord* _firstNotTransferredMarker1 = nullptr;
	DataAllocationRecord* _firstNotTransferredMarker2 = nullptr;

	inline DataMemory(DataStorage& dataStorage);
	void releaseMemoryMarker1Chain(DataAllocationRecord* a);
	void releaseMemoryMarker2Chain(DataAllocationRecord* a);
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
	inline DataStorage& dataStorage() const;
	inline size_t size() const;
	inline vk::Buffer buffer() const;
	inline vk::DeviceMemory memory() const;
	inline vk::DeviceAddress deviceAddress() const;
	inline size_t usedBytes() const;

	// low-level allocation functions
	// (mostly for internal use)
	DataAllocationRecord* alloc(size_t numBytes);
	static inline void free(DataAllocationRecord* a) noexcept;
	void cancelAllAllocations();
	[[nodiscard]] std::tuple<void*,void*,size_t> recordUploads(vk::CommandBuffer);
	void uploadDone(void*, void*) noexcept;

};


}

#endif


// inline methods
#if !defined(CADR_DATA_MEMORY_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_DATA_MEMORY_INLINE_FUNCTIONS
namespace CadR {

inline DataMemory::DataMemory(DataStorage& dataStorage) : _dataStorage(&dataStorage)  {}
inline DataStorage& DataMemory::dataStorage() const  { return *_dataStorage; }
inline size_t DataMemory::size() const  { return _bufferEndAddress - _bufferStartAddress; }
inline vk::Buffer DataMemory::buffer() const  { return _buffer; }
inline vk::DeviceMemory DataMemory::memory() const  { return _memory; }
inline vk::DeviceAddress DataMemory::deviceAddress() const  { return _bufferStartAddress; }
inline size_t DataMemory::usedBytes() const  { return _usedBytes; }
inline void DataMemory::free(DataAllocationRecord* a) noexcept  { a->dataMemory->freeInternal(a); }

}
#endif
