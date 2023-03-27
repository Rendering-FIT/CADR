#pragma once

#include <cstddef>

namespace CadR {

class StagingDataAllocation;


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
	StagingDataAllocation* _stagingDataAllocation = nullptr;
	StagingData() = default;
public:

	// construction, initialization and destruction
	StagingData(StagingDataAllocation* owner);
	StagingData(std::nullptr_t);
	~StagingData() noexcept;

	// deleted constructors and operators
	StagingData(const StagingData&) = delete;
	StagingData(StagingData&&) = delete;
	StagingData& operator=(const StagingData&) = delete;
	StagingData& operator=(StagingData&&) = delete;

	// getters
	template<typename T = void>
	T* data();
	size_t size() const;

	// functions
	void submit();
	void cancel();

};


}


// inline methods
#include <CadR/StagingDataAllocation.h>
#include <CadR/DataAllocation.h>
namespace CadR {

inline StagingData::~StagingData() noexcept  { if(_stagingDataAllocation) _stagingDataAllocation->_referenceCounter--; }
inline StagingData::StagingData(StagingDataAllocation* owner) : StagingData()  { owner->_referenceCounter++; _stagingDataAllocation = owner; }
inline StagingData::StagingData(std::nullptr_t) : StagingData()  { _stagingDataAllocation = nullptr; }
inline void StagingData::cancel()  { if(_stagingDataAllocation) { _stagingDataAllocation->_referenceCounter--; _stagingDataAllocation = nullptr; } }
template<typename T>
inline T* StagingData::data()  { return reinterpret_cast<T*>(_stagingDataAllocation->_data); }

}
