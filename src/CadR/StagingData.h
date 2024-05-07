#pragma once

#include <cstddef>

namespace CadR {

struct DataAllocationRecord;


/** \brief StagingData is used to update data in particular DataAllocation
 *  using stagging buffer approach.
 *
 *  It represents single piece of memory associated with
 *  particular DataAllocation and StagingDataAllocation.
 *  The user updates the memory and after calling submit(), copy operation
 *  is scheduled to update memory associated with particular DataAllocation.
 *
 *  \sa StagingDataAllocation, DataAllocation
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
