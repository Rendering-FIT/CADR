#include <CadR/DataStorage.h>
#include <CadR/DataMemory.h>
#include <CadR/Exceptions.h>
#include <CadR/Renderer.h>
#include <CadR/StagingMemory.h>
#include <CadR/StagingBuffer.h>
#include <CadR/VulkanDevice.h>
#include <iostream> // for cerr
#include <memory>
#include <numeric>

using namespace std;
using namespace CadR;



void DataStorage::destroy() noexcept
{
	// destroy StagingDataAllocations in _pendingList
	while(!_pendingList.empty()) {
		auto& l = _pendingList.front();
		while(!l.empty())
			l.front().free();
		_pendingList.pop_front();
	}

	// destroy StagingDataAllocations in _submittedList
	while(!_submittedList.empty())
		_submittedList.front().free();

	// destroy StagingMemory objects
	for(StagingMemory* m : _stagingMemoryList)
		delete m;
	_stagingMemoryList.clear();

	// destroy DataMemory objects
	for(DataMemory* m : _dataMemoryList)
		delete m;
	_dataMemoryList.clear();
	_firstAllocMemory = nullptr;
	_secondAllocMemory = nullptr;
}


DataStorage::~DataStorage() noexcept
{
	destroy();
}


DataStorage::DataStorage()
	: DataStorage(*Renderer::get())
{
}


DataStorage::DataStorage(Renderer& renderer)
	: _renderer(&renderer)
{
}


DataAllocation* DataStorage::alloc(size_t numBytes, MoveCallback moveCallback, void* moveCallbackUserData)
{
	// make sure we have _firstAllocMemory
	// (it might be missing just before the first call to alloc())
	if(_firstAllocMemory == nullptr) {
		size_t size =
			(numBytes < _renderer->bufferSizeList()[0])
				? _renderer->bufferSizeList()[0]
				: (numBytes < _renderer->bufferSizeList()[1])
					? _renderer->bufferSizeList()[1]
					: max(numBytes, _renderer->bufferSizeList()[2]);
		_firstAllocMemory = DataMemory::tryCreate(*this, size);
		if(_firstAllocMemory == nullptr)
			throw OutOfResources("CadR::DataStorage::alloc() error: Cannot allocate DataMemory. "
			                     "Requested size: " + to_string(size) + " bytes.");
		_dataMemoryList.emplace_back(_firstAllocMemory);
	}

	// try alloc from _firstAllocMemory
	DataAllocation* a = _firstAllocMemory->alloc(numBytes, moveCallback, moveCallbackUserData);
	if(a)
		return a;

	// make sure we have _secondAllocMemory
	// (it might be missing until the first DataMemory is full)
	if(_secondAllocMemory == nullptr) {
		size_t size =
			(numBytes < _renderer->bufferSizeList()[1])
				? _renderer->bufferSizeList()[1]
				: max(numBytes, _renderer->bufferSizeList()[2]);
		_secondAllocMemory = DataMemory::tryCreate(*this, size);
		if(_secondAllocMemory == nullptr)
			throw OutOfResources("CadR::DataStorage::alloc() error: Cannot allocate DataMemory. "
			                     "Requested size: " + to_string(size) + " bytes.");
		_dataMemoryList.emplace_back(_secondAllocMemory);
	}

	// the alloc from _secondAllocMemory
	a = _secondAllocMemory->alloc(numBytes, moveCallback, moveCallbackUserData);
	if(a)
		return a;

	// create new DataMemory
	// and set is as _secondAllocMemory
	// (_firstAllocMemory is considered full now, _secondAllocMemory almost full,
	// so we replace _firstAlloc memory by _secondAllocMemory and
	// we put new DataMemory into _secondAllocMemory)
	size_t size = max(_renderer->bufferSizeList()[2], numBytes);
	DataMemory* m = DataMemory::tryCreate(*this, size);
	if(m == nullptr)
		throw OutOfResources("CadR::DataStorage::alloc() error: Cannot allocate DataMemory. "
		                     "Requested size: " + to_string(size) + " bytes.");
	_dataMemoryList.emplace_back(m);
	_firstAllocMemory = _secondAllocMemory;
	_secondAllocMemory = m;

	// make the allocation
	// from the new DataMemory
	return _secondAllocMemory->alloc(numBytes, moveCallback, moveCallbackUserData);
}


void DataStorage::cancelAllAllocations()
{
	for(DataMemory* m : _dataMemoryList)
		m->cancelAllAllocations();
}


StagingData DataStorage::createStagingData(DataAllocation* a)
{
	assert(a != nullptr && "DataStorage::createStagingData(): Passed null parameter.");

	// return already attached StagingDataAllocation
	if(a->_stagingDataAllocation)
		return StagingData(a->_stagingDataAllocation);

	// try to use existing StagingMemories
	if(!_stagingMemoryList.empty())
	{
		// get the last StagingMemory,
		// but use only first three in the list
		size_t index = _stagingMemoryList.size() - 1;
		if(index > 2)
			index = 2;

		// try alloc
		StagingDataAllocation* s = _stagingMemoryList[index]->alloc(a);
		if(s)
			return StagingData(s);
	}

	// try to use first three StagingMemories
	// while allocating them if they do not exist
	for(size_t i=_stagingMemoryList.size(); i<3; i++)
	{
		_stagingMemoryList.push_back(new StagingMemory(*this, _renderer->bufferSizeList()[i]));

		// try alloc
		StagingDataAllocation* s = _stagingMemoryList.back()->alloc(a);
		if(s)
			return StagingData(s);
	}

	// try to use remaining StagingMemories
	for(size_t i=3, c=_stagingMemoryList.size(); i<c; i++)
	{
		// try alloc
		StagingDataAllocation* s = _stagingMemoryList.back()->alloc(a);
		if(s)
			return StagingData(s);
	}

	// alloc extra StagingMemory
	_stagingMemoryList.push_back(new StagingMemory(*this, max(a->size(), _renderer->bufferSizeList()[2])));

	// try alloc
	StagingDataAllocation* s = _stagingMemoryList.back()->alloc(a);
	if(s)
		return StagingData(s);

	throw LogicError("CadR::DataStorage::createStagingData(): Failed to allocate StagingData (size: " + to_string(a->size()) + " bytes).");
}


DataStorage::UploadSetId DataStorage::recordUpload(vk::CommandBuffer commandBuffer)
{
	if(_submittedList.empty())
		return _pendingList.end();

	VulkanDevice* device = _renderer->device();
	_pendingList.emplace_back();
	UploadSetId it = prev(_pendingList.end()); 
	do {
		StagingDataAllocation& s = _submittedList.front();
		device->cmdCopyBuffer(
			commandBuffer,  // commandBuffer
			s.stagingDataMemory().buffer(),  // srcBuffer
			s.destinationDataMemory().buffer(),  // dstBuffer
			vk::BufferCopy(  // regions
				s.stagingOffset(),  // srcOffset
				s.destinationOffset(),  // dstOffset
				s.size())  // size
		);
		_submittedList.pop_front();
		it->push_back(s);
	}
	while(!_submittedList.empty());
	return it;
}


void DataStorage::disposeUploadSet(DataStorage::UploadSetId uploadSetId)
{
	if(uploadSetId == _pendingList.end())
		return;
	while(!uploadSetId->empty())
		uploadSetId->front().free();
	_pendingList.erase(uploadSetId);
}


int DataStorage::getHighestUsedStagingMemory() const
{
	for(int i=int(_stagingMemoryList.size())-1; i>=0; i--)
		if(_stagingMemoryList[i]->usedBytes() > 0)
			return i;
	return -1;
}


void DataStorage::disposeStagingMemories(int fromIndexUp)
{
	if(fromIndexUp >= int(_stagingMemoryList.size()))
		return;
	if(fromIndexUp < 0)
		fromIndexUp = 0;
	for(int i=int(_stagingMemoryList.size())-1; i>=fromIndexUp; i--)
	{
		StagingMemory* m = _stagingMemoryList[i];
		if(m->usedBytes() > 0)
			throw LogicError("CadR::DataStorage::disposeStagingMemories(): Cannot dispose StagingMemory object because it still holds data.");
		delete m;
		_stagingMemoryList.pop_back();
	}
}
