#include <CadR/DataAllocation.h>
#include <CadR/DataStorage.h>
#include <CadR/Renderer.h>
#include <CadR/StagingData.h>

using namespace std;
using namespace CadR;


DataAllocationRecord DataAllocationRecord::nullRecord{ 0, 0, nullptr, nullptr, nullptr, size_t(-2) };



StagingData DataAllocation::alloc(size_t numBytes)
{
	DataStorage& storage = _record->dataMemory->dataStorage();
	Renderer& r = storage.renderer();

	// reuse current allocation
	// (it was allocated earlier in this frame)
	if(_record->stagingFrameNumber == r.frameNumber() && numBytes <= _record->size) {
		_record->size = numBytes;
		return StagingData(_record, false);
	}

	// re-allocate DataAllocation and its staging data
	_record = storage.realloc(_record, numBytes);
	storage.setHandle(_handle, _record->deviceAddress);
	return StagingData(_record, true);
}


StagingData DataAllocation::alloc()
{
	DataStorage& storage = _record->dataMemory->dataStorage();
	Renderer& r = storage.renderer();

	// return current staging data
	// (they were allocated earlier in this frame)
	if(_record->stagingFrameNumber == r.frameNumber())
		return StagingData(_record, false);

	// re-allocate DataAllocation and its staging data
	_record = storage.realloc(_record, _record->size);
	storage.setHandle(_handle, _record->deviceAddress);
	return StagingData(_record, true);
}


StagingData HandlelessAllocation::alloc(size_t size)
{
	DataStorage& storage = _record->dataMemory->dataStorage();
	Renderer& r = storage.renderer();

	// reuse current allocation
	// (it was allocated earlier in this frame)
	if(_record->stagingFrameNumber == r.frameNumber() && size <= _record->size) {
		_record->size = size;
		return StagingData(_record, false);
	}

	// re-allocate DataAllocation and its staging data
	_record = storage.realloc(_record, size);
	return StagingData(_record, true);
}


StagingData HandlelessAllocation::alloc()
{
	DataStorage& storage = _record->dataMemory->dataStorage();
	Renderer& r = storage.renderer();

	// reuse current allocation
	// (it was allocated earlier in this frame)
	if(_record->stagingFrameNumber == r.frameNumber())
		return StagingData(_record, false);

	// re-allocate DataAllocation and its staging data
	_record = storage.realloc(_record, _record->size);
	return StagingData(_record, true);
}


void DataAllocation::upload(const void* ptr, size_t numBytes)
{
	DataStorage& storage = _record->dataMemory->dataStorage();
	_record = storage.realloc(_record, numBytes);
	storage.setHandle(_handle, _record->deviceAddress);
	memcpy(_record->stagingData, ptr, numBytes);
}


void HandlelessAllocation::upload(const void* ptr, size_t numBytes)
{
	_record = dataStorage().realloc(_record, numBytes);
	memcpy(_record->stagingData, ptr, numBytes);
}
