#include <CadR/StagingData.h>
#include <CadR/DataAllocation.h>
#include <CadR/DataStorage.h>

using namespace std;
using namespace CadR;



void StagingData::submit()
{
	if(_stagingDataAllocation == nullptr)
		throw runtime_error("StagingData::submit(): No staging data allocated.");

	// do nothing unless we are the last StagingData object
	_stagingDataAllocation->_referenceCounter--;
	if(_stagingDataAllocation->_referenceCounter != 0)
		return;

	// enqueue StagingDataAllocation
	DataStorage& s = _stagingDataAllocation->_owner->dataStorage();
	s._submittedList.push_back(*_stagingDataAllocation);
	_stagingDataAllocation = nullptr;
}
