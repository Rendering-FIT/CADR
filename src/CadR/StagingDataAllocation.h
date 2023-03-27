#pragma once

#include <cstdint>
#include <boost/intrusive/list.hpp>

namespace CadR {

class DataAllocation;
class DataMemory;
class DataStorage;
class StagingMemory;


/** \brief StagingDataAllocation represents allocated memory that is used
 *  to update data in particular DataAllocation using stagging buffer approach.
 *
 *  It represents single piece of memory associated with particular DataAllocation.
 *  StagingData class is usually used as front-end for this class.
 *
 *  \sa StagingData, DataAllocation
 */
class CADR_EXPORT StagingDataAllocation {
protected:

	void* _data;
	DataAllocation* _owner;
	StagingMemory* _stagingMemory;
	unsigned _referenceCounter;
	boost::intrusive::list_member_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink>
	> _submittedListHook;  ///< List hook of DataStorage::_submittedList.

	friend DataAllocation;
	friend class StagingData;
	friend StagingMemory;
	friend DataStorage;
	StagingDataAllocation(void* data, DataAllocation* owner, StagingMemory* stagingMemory, unsigned referenceCounter);

public:

	// construction, initialization and destruction
	StagingDataAllocation() = default;
	~StagingDataAllocation() noexcept;
	void init(uint64_t address, DataAllocation* a, StagingMemory* s);
	void free();

	// deleted constructors and operators
	StagingDataAllocation(const StagingDataAllocation&) = delete;
	StagingDataAllocation(StagingDataAllocation&&) = delete;
	StagingDataAllocation& operator=(const StagingDataAllocation&) = delete;
	StagingDataAllocation& operator=(StagingDataAllocation&&) = delete;

	// getters
	template<typename T = void>
	T* data();
	size_t size() const;
	DataStorage& dataStorage() const;
	StagingMemory& stagingDataMemory() const;
	DataMemory& destinationDataMemory() const;
	size_t stagingOffset() const;
	size_t destinationOffset() const;

};


}


// inline methods
#include <CadR/DataAllocation.h>
#include <CadR/StagingMemory.h>
namespace CadR {

inline StagingDataAllocation::StagingDataAllocation(void* data, DataAllocation* owner, StagingMemory* stagingMemory, unsigned referenceCounter) : _data(data), _owner(owner), _stagingMemory(stagingMemory), _referenceCounter(referenceCounter)  {}
inline void StagingDataAllocation::init(uint64_t address, DataAllocation* a, StagingMemory* s)  { _data = reinterpret_cast<void*>(address); _owner = a; _stagingMemory = s; _referenceCounter = 0; }
template<typename T>
inline T* StagingDataAllocation::data()  { return reinterpret_cast<T*>(_data); }
inline StagingMemory& StagingDataAllocation::stagingDataMemory() const  { return *_stagingMemory; }

}
