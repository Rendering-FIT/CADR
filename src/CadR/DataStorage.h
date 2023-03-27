#pragma once

#include <array>
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <boost/intrusive/list.hpp>
#include <CadR/StagingData.h>
#include <CadR/StagingDataAllocation.h>

namespace CadR {

class DataAllocation;
class DataMemory;
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
public:
	using MoveCallback = void (*)(DataAllocation* oldAlloc, DataAllocation* newAlloc, void* userData);
	typedef boost::intrusive::list<
		StagingDataAllocation,
		boost::intrusive::member_hook<
			StagingDataAllocation,
			boost::intrusive::list_member_hook<
				boost::intrusive::link_mode<boost::intrusive::auto_unlink>>,
			&StagingDataAllocation::_submittedListHook>,
		boost::intrusive::constant_time_size<false>
	> StagingDataAllocationList;
	typedef std::list<StagingDataAllocationList>::iterator UploadSetId;
protected:

	Renderer* _renderer;  ///< Rendering context associated with the DataStorage.
	std::vector<std::unique_ptr<DataMemory>> _dataMemoryList;
	DataMemory* _firstAllocMemory = nullptr;
	DataMemory* _secondAllocMemory = nullptr;
	std::array<StagingMemory*, 3> _stagingMemoryList = {};
	StagingDataAllocationList _submittedList;
	std::list<StagingDataAllocationList> _pendingList;

	friend DataMemory;
	friend StagingData;

public:

	// construction and destruction
	DataStorage();
	DataStorage(Renderer* r);
	~DataStorage() noexcept;
	void destroy() noexcept;

	// deleted constructors and operators
	DataStorage(const DataStorage&) = delete;
	DataStorage(DataStorage&&) = delete;
	DataStorage& operator=(const DataStorage&) = delete;
	DataStorage& operator=(DataStorage&&) = delete;

	// getters
	std::vector<std::unique_ptr<DataMemory>>& dataMemoryList();  ///< Returns DataMemory list. It is generally better to use standard Geometry's alloc/realloc/free methods than trying to modify the list directly.
	const std::vector<std::unique_ptr<DataMemory>>& dataMemoryList() const;  ///< Returns DataMemory list.
	Renderer* renderer() const;

	// methods
	DataAllocation* alloc(size_t numBytes, MoveCallback moveCallback, void* moveCallbackUserData);
	static void free(DataAllocation* a);
	void cancelAllAllocations();
	StagingData createStagingData(DataAllocation* a);
	UploadSetId recordUpload(vk::CommandBuffer commandBuffer);
	void disposeUploadSet(UploadSetId uploadSet);

};


}


// inline and template methods
#include <CadR/DataMemory.h>
namespace CadR {

inline std::vector<std::unique_ptr<DataMemory>>& DataStorage::dataMemoryList()  { return _dataMemoryList; }
inline const std::vector<std::unique_ptr<DataMemory>>& DataStorage::dataMemoryList() const  { return _dataMemoryList; }
inline Renderer* DataStorage::renderer() const  { return _renderer; }

inline void DataStorage::free(DataAllocation* a)  { DataMemory::free(a); }

}
