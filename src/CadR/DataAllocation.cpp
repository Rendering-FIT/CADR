#include <CadR/DataAllocation.h>
#include <CadR/DataStorage.h>
#include <CadR/StagingData.h>

using namespace std;
using namespace CadR;



DataAllocation* DataAllocation::alloc(DataStorage& storage, size_t size,
                                      MoveCallback moveCallback, void* moveCallbackUserData)
{
	return storage.alloc(size, moveCallback, moveCallbackUserData);
}


StagingData DataAllocation::createStagingData()
{
	if(_stagingDataAllocation)
		return StagingData(_stagingDataAllocation);
	else
		return dataStorage().createStagingData(this);
}


void DataAllocation::upload(void* data)
{
	StagingData s = createStagingData();
	memcpy(s.data(), data, _size);
	s.submit();
}
