#pragma once

#include <cstddef>

namespace CadR {

struct DataAllocationRecord;


/** \brief StagingData is used to update data in particular DataAllocation
 *  or HandlelessAllocation using stagging buffer approach.
 *
 *  It represents single piece of memory allocated in StagingMemory object.
 *  StagingData object is created by calling DataAllocation::alloc()
 *  or HandlelessAllocation::alloc(). The user is expected to update
 *  the memory pointed by data() until the next frame rendering, or until
 *  the next call to Renderer::submitCopyOperations(). The whole memory block
 *  is expected to be updated, otherwise undefined content will be uploaded
 *  from all not updated parts of the memory.
 *
 *  After next frame rendering process is started or after the call to
 *  Renderer::submitCopyOperations() StagingData object is invalid
 *  and shall not be used any more.
 *
 *  \sa DataAllocation, HandlelessAllocation, DataAllocationRecord
 */
class CADR_EXPORT StagingData {
protected:
	DataAllocationRecord* _record;
	bool _needInit;
public:

	// construction, initialization and destruction
	StagingData() = default;
	StagingData(DataAllocationRecord* record, bool needInit);

	// getters
	template<typename T = void> T* data();
	size_t sizeInBytes() const;
	bool needInit() const;

};


}


// inline methods
namespace CadR {

inline StagingData::StagingData(DataAllocationRecord* record, bool needInit) : _record(record), _needInit(needInit)  {}
inline bool StagingData::needInit() const  { return _needInit; }

}


// include inline StagingData functions defined in DataAllocation.h (they are defined there because they depend on DataAllocationRecord struct)
#ifndef CADR_NO_DATAALLOCATION_INCLUDE
# include <CadR/DataAllocation.h>
#endif
