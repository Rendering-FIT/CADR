#include <CadR/DataStorage.h>
#include <CadR/Exceptions.h>
#include <CadR/Renderer.h>
#include <CadR/StagingMemory.h>
#include <CadR/VulkanDevice.h>

using namespace std;
using namespace CadR;



void DataStorage::destroy() noexcept
{
	// destroy all handles
	_handleTable.destroyAll();

	// destroy StagingMemory objects
	_stagingMemoryRegister.inUse64KiBList.clear();
	_stagingMemoryRegister.available64KiBList.clear();
	_stagingMemoryRegister.inUse2MiBList.clear();
	_stagingMemoryRegister.available2MiBList.clear();
	_stagingMemoryRegister.inUse32MiBList.clear();
	_stagingMemoryRegister.available32MiBList.clear();
	_stagingMemoryRegister.inUseSuperSizeList.clear();
	_stagingMemoryRegister.availableSuperSizeList.clear();

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


DataStorage::DataStorage(Renderer& renderer)
	: _renderer(&renderer)
	, _handleTable(*this)
{
}


DataAllocationRecord* DataStorage::allocInternal(size_t numBytes)
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
	DataAllocationRecord* a = _firstAllocMemory->alloc(numBytes);
	if(a == nullptr) {

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
		a = _secondAllocMemory->alloc(numBytes);
		if(a == nullptr) {

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
			a = _secondAllocMemory->alloc(numBytes);
			if(a == nullptr)
				throw OutOfResources("CadR::DataStorage::alloc() error: Cannot allocate DataAllocation "
				                     "although new DataMemory was already created. "
				                     "Requested size: " + to_string(size) + " bytes.");
		}
	}

	return a;
}


/** Performs allocation and returns pointer to the new DataAllocation object.
 *
 *  DataStorage maintains the collection of DataMemory objects.
 *  The real allocation is returned from one of these DataMemory objects
 *  while DataStorage serves just as a manager of DataMemory objects.
 *
 *  Internally, the function uses the following strategy:
 *  It makes three allocation attempts, using _firstAllocMemory,
 *  using _secondAllocMemory, and using new DataMemory object.
 */
DataAllocationRecord* DataStorage::alloc(size_t numBytes)
{
#if 0
	// disallow zero size allocations
	if(numBytes == 0)
		throw LogicError("DataStorage::alloc(): The size parameter must not be 0.");
#else
	// null object pattern
	// (single zero size allocation object is provided to avoid resource
	// consumption of empty allocations; zero size object also holds
	// pointer to DataStorage object which might be very useful in some cases)
	if(numBytes == 0)
		return &_zeroSizeAllocationRecord;
#endif

	// alloc record
	// (the function either succeeds or throws)
	DataAllocationRecord* a = allocInternal(numBytes);

	// alloc staging data
	// (allocInternal already initialized deviceAddress, size and dataMemory members)
	try {
		uint64_t offsetInBuffer = a->deviceAddress - a->dataMemory->deviceAddress();
		a->stagingData = stagingAlloc(a, offsetInBuffer);  // this might throw
		a->stagingFrameNumber = _renderer->frameNumber();
	} catch(...) {
		DataMemory::free(a);
		throw;
	}

	// return pointer to AllocationRecord
	return a;
}


/** Performs reallocation of DataAllocation object.
 *
 *  Old allocation is destroyed and new is returned, having new size, new device address, but the same handle.
 *  If the function throws, it leaves the old allocation unchanged.
 */
DataAllocationRecord* DataStorage::realloc(DataAllocationRecord* allocationRecord, size_t numBytes)
{
#if 0
	// disallow zero size allocations
	if(numBytes == 0)
		throw LogicError("DataStorage::realloc(): The size parameter must not be 0.");
#else
	// null object pattern
	// (single zero size allocation object is provided to avoid resource
	// consumption of empty allocations; zero size object also holds
	// pointer to DataStorage object which might be very useful in some cases)
	if(numBytes == 0) {
		DataStorage::free(allocationRecord);
		return &_zeroSizeAllocationRecord;
	}
#endif

	// alloc record
	// (the function either succeeds or throws)
	DataAllocationRecord* a = allocInternal(numBytes);

	// alloc staging data
	// (allocInternal already initialized deviceAddress, size and dataMemory members)
	try {
		uint64_t offsetInBuffer = a->deviceAddress - a->dataMemory->deviceAddress();
		a->stagingData = stagingAlloc(a, offsetInBuffer);  // this might throw
		a->stagingFrameNumber = _renderer->frameNumber();
	} catch(...) {
		DataMemory::free(a);
		throw;
	}

	// free old allocationRecord
	if(allocationRecord->size != 0)
		DataMemory::free(allocationRecord);  // does not throw

	// return pointer to AllocationRecord
	return a;
}


void DataStorage::cancelAllAllocations()
{
	for(DataMemory* m : _dataMemoryList)
		m->cancelAllAllocations();
}


/** Performs allocation and returns new StagingData object.
 *  The returned object is used for uploading data from CPU memory
 *  to GPU memory. Data are stored into StagingData object and
 *  uploaded to DataAllocation object.
 *
 *  DataStorage maintains the collection of StagingMemory objects.
 *  The real allocation is returned from one of these StagingMemory objects
 *  while DataStorage serves just as a manager of StagingMemory objects.
 *
 *  If staging memory was already created for particular DataAllocation object,
 *  new staging memory is not created. Instead, already existing memory is returned.
 *
 *  Internally, the function uses the following strategy:
 *  It returns previously allocated staging memory if one exists.
 *  Otherwise, it attempts to use first three StagingMemories.
 *  If no success, it tries remaining StagingMemories as well.
 *  As the last attempt, it tries to allocate new StagingMemory object
 *  of the size bigger or equal to the requested staging memory size.
 */
void* DataStorage::stagingAlloc(DataAllocationRecord* allocationRecord, uint64_t offsetInBuffer)
{
	// Because we are not suballocating the memory here (this is intention),
	// but allocating small buffers first (to stay on low memory requirements for small scenes),
	// we need to aggressively increase buffer sizes when more staging memory is needed
	// to not reach VkPhysicalDeviceLimits::maxMemoryAllocationCount limit
	// that is guarantted to be at least 4096.
	// At the same time, we do not want to create too big buffers because of
	// the performance cost of their creation. Teens of milliseconds will already mean
	// loosing one or more frames.
	//
	// Using 32MiB buffers means we can allocate 1024 of them on 32GiB systems,
	// leaving still 3072 allocations as reserve.

	size_t numBytes = allocationRecord->size;
	assert(numBytes > 0 && "DataStorage::stagingAlloc() called on allocation of zero size.");
	DataMemory& dataMemory = *allocationRecord->dataMemory;

	// try alloc from dataMemory._stagingMemoryList.back()
	if(!dataMemory._stagingMemoryList.empty()) {
		StagingMemory* m = dataMemory._stagingMemoryList.back();
		uint64_t addr = m->alloc(offsetInBuffer, numBytes);
		if(addr)
			return reinterpret_cast<void*>(addr);
	}

	// sum amount of already allocated staging memories
	size_t alreadyAllocated = 0;
	for(StagingMemory* sm : dataMemory._stagingMemoryList)
		alreadyAllocated += sm->size();

	// reuse or create StagingMemory
	auto allocStagingMemory =
		[](list<StagingMemory>& availableList, list<StagingMemory>& inUseList, Renderer& r, size_t numBytes, size_t capacityHint) -> StagingMemory* {
			for(auto it=availableList.begin(); it!=availableList.end(); it++)
				if(it->size() >= numBytes) {
					inUseList.splice(inUseList.end(), availableList, it);
					return &*it;
				}
			return &inUseList.emplace_back(r, capacityHint);  // this might throw, especially in StagingMemory constructor
		};

	// alloc StagingMemory of appropriate size
	StagingMemory* m;
	if(_stagingDataSizeHint <= 256*1024 && alreadyAllocated < 256*1024 && numBytes <= 64*1024)
		m = allocStagingMemory(_stagingMemoryRegister.available64KiBList, _stagingMemoryRegister.inUse64KiBList, *_renderer, numBytes, 64*1024);  // this might throw
	else if(_stagingDataSizeHint <= 8*1048576 && alreadyAllocated < 8*1048576 && numBytes <= 2*1048576)
		m = allocStagingMemory(_stagingMemoryRegister.available2MiBList, _stagingMemoryRegister.inUse2MiBList, *_renderer, numBytes, 2*1048576);  // this might throw
	else if(numBytes <= 32*1048576)
		m = allocStagingMemory(_stagingMemoryRegister.available32MiBList, _stagingMemoryRegister.inUse32MiBList, *_renderer, numBytes, 32*1048576);  // this might throw
	else {
		// find the first suitable StagingMemory
		// in super size list
		auto it = _stagingMemoryRegister.availableSuperSizeList.begin();
		auto e = _stagingMemoryRegister.availableSuperSizeList.end();
		while(true) {
			if(it == e) {
				// alloc new super size StagingMemory
				m = &_stagingMemoryRegister.inUseSuperSizeList.emplace_back(*_renderer, numBytes);  // this might throw
				goto found;
			}
			// break on first suitable StagingMemory
			if(it->size() >= numBytes)
				break;

			it++;
		}
		auto bestMemory = it;
		size_t bestSize = it->size();

		// find the best suitable StagingMemory
		// in super size list
		for(it++; it!=e; it++) {
			size_t s = it->size();
			if(s >= numBytes && s < bestSize) {
				bestMemory = it;
				bestSize = s;
			}
		}
		_stagingMemoryRegister.inUseSuperSizeList.splice(_stagingMemoryRegister.inUseSuperSizeList.end(), _stagingMemoryRegister.availableSuperSizeList, bestMemory);
		m = &*bestMemory;
	}

	// alloc piece of staging memory
found:
	m->attach(dataMemory, allocationRecord->deviceAddress - dataMemory.deviceAddress());  // this might theoretically throw
	uint64_t addr = m->alloc(offsetInBuffer, numBytes);
	if(addr)
		return reinterpret_cast<void*>(addr);
	else
		throw OutOfResources("Cannot allocate staging memory.");
}


tuple<DataStorage::TransferRecord,size_t>
	DataStorage::recordUpload(vk::CommandBuffer commandBuffer)
{
	size_t bytesToTransfer = 0;
	auto it = _transferInProgressList.emplace(_transferInProgressList.end());
	TransferRecord tr(it, this);
	for(DataMemory* dm : _dataMemoryList)
		for(StagingMemory* sm : dm->_stagingMemoryList) {
			auto [ id, numBytes ] = sm->recordUpload(commandBuffer);
			it->emplace_back(sm, id);
			bytesToTransfer += numBytes;
		}
	return { move(tr), bytesToTransfer };
}


void DataStorage::uploadDone(TransferId id) noexcept
{
	// verify valid usage
	assert(id == _transferInProgressList.begin() && "DataStorage::uploadDone(TransferId) must be called on TransferIds "
	                                                "in the same order as they are returned by DataStorage::recordUpload().");

	// notify all StagingMemories
	for(auto& r : *id) {
		StagingMemory* sm = std::get<0>(r);
		uint64_t id2 = std::get<1>(r);
		sm->uploadDone(id2);
		if(sm->canDetach()) {
			sm->detach();
			list<StagingMemory>* srcList;
			list<StagingMemory>* dstList;
			if(sm->size() <= 64*1024) {
				srcList = &_stagingMemoryRegister.inUse64KiBList;
				dstList = &_stagingMemoryRegister.available64KiBList;
			}
			else if(sm->size() <= 2*1048576) {
				srcList = &_stagingMemoryRegister.inUse2MiBList;
				dstList = &_stagingMemoryRegister.available2MiBList;
			}
			else if(sm->size() <= 32*1048576) {
				srcList = &_stagingMemoryRegister.inUse32MiBList;
				dstList = &_stagingMemoryRegister.available32MiBList;
			}
			else {
				srcList = &_stagingMemoryRegister.inUseSuperSizeList;
				dstList = &_stagingMemoryRegister.availableSuperSizeList;
			}
			for(auto it=srcList->begin(); it!=srcList->end(); it++)
				if(&(*it) == sm) {
					dstList->splice(dstList->begin(), *srcList, it);
					break;
				}
		}
	}

	// remove transfer from the list
	_transferInProgressList.erase(id);
}
