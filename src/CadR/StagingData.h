#ifndef CADR_STAGING_DATA_HEADER
# define CADR_STAGING_DATA_HEADER

# include <cstddef>

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
	inline StagingData(DataAllocationRecord* record, bool needInit);

	// getters
	template<typename T = void> inline T* data();
	inline size_t sizeInBytes() const;
	inline bool needInit() const;

};


}

#endif


// inline methods
#if !defined(CADR_STAGING_DATA_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_STAGING_DATA_INLINE_FUNCTIONS
# define CADR_NO_INLINE_FUNCTIONS
# include <CadR/DataAllocation.h>
# undef CADR_NO_INLINE_FUNCTIONS
namespace CadR {

inline StagingData::StagingData(DataAllocationRecord* record, bool needInit) : _record(record), _needInit(needInit)  {}
template<typename T> inline T* StagingData::data()  { return reinterpret_cast<T*>(_record->stagingData); }
inline size_t StagingData::sizeInBytes() const  { return _record->size; }
inline bool StagingData::needInit() const  { return _needInit; }

}
#endif
