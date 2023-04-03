#include <CadR/DataStorage.h>
#include <CadR/DataMemory.h>
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
	for(auto it = _stagingMemoryList.begin(); it != _stagingMemoryList.end(); it++) {
		delete *it;
		*it = nullptr;
	}

	// destroy VertexMemory objects
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
		_firstAllocMemory = DataMemory::tryCreate(*this,
			max(_renderer->bufferSizeList()[0], numBytes));
		if(_firstAllocMemory == nullptr)
			return nullptr;
		_dataMemoryList.emplace_back(_firstAllocMemory);
	}

	// try alloc from _firstAllocMemory
	DataAllocation* a = _firstAllocMemory->alloc(numBytes, moveCallback, moveCallbackUserData);
	if(a)
		return a;

	// make sure we have _secondAllocMemory
	// (it might be missing until the first DataMemory is full)
	if(_secondAllocMemory == nullptr) {
		_secondAllocMemory = DataMemory::tryCreate(*this,
			max(_renderer->bufferSizeList()[1], numBytes));
		if(_secondAllocMemory == nullptr)
			return nullptr;
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
	DataMemory* m = DataMemory::tryCreate(*this, max(_renderer->bufferSizeList()[2], numBytes));
	if(m == nullptr)
		return nullptr;
	_dataMemoryList.emplace_back(m);
	_firstAllocMemory = _secondAllocMemory;
	_secondAllocMemory = m;

	// make the allocation
	// from new _secondAllocMemory
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

	// get index of StagingMemory
	size_t index = _dataMemoryList.size();
	if(index > 0)
		index--;
	if(index > 2)
		index = 2;

	// alloc StagingMemory
	if(_stagingMemoryList[index] == nullptr)
		for(size_t i=0; i<=index; i++)
			if(_stagingMemoryList[i] == nullptr)
				_stagingMemoryList[i] = new StagingMemory(*this, _renderer->bufferSizeList()[i]);

	// try alloc
	StagingDataAllocation* s = _stagingMemoryList[index]->alloc(a);
	if(s)
		return StagingData(s);

	return StagingData(nullptr);
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
