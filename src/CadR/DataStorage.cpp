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

	// destroy DataMemory objects
	for(DataMemory* m : _dataMemoryList)
		delete m;
	_dataMemoryList.clear();
	_firstAllocMemory = nullptr;
	_secondAllocMemory = nullptr;

	// destroy StagingMemory objects
	_stagingMemoryRegister.smallMemoryInUseList.clear_and_dispose(StagingMemoryRegister::Disposer());
	_stagingMemoryRegister.smallMemoryAvailableList.clear_and_dispose(StagingMemoryRegister::Disposer());
	_stagingMemoryRegister.middleMemoryInUseList.clear_and_dispose(StagingMemoryRegister::Disposer());
	_stagingMemoryRegister.middleMemoryAvailableList.clear_and_dispose(StagingMemoryRegister::Disposer());
	_stagingMemoryRegister.largeMemoryInUseList.clear_and_dispose(StagingMemoryRegister::Disposer());
	_stagingMemoryRegister.largeMemoryAvailableList.clear_and_dispose(StagingMemoryRegister::Disposer());
	_stagingMemoryRegister.superSizeMemoryInUseList.clear_and_dispose(StagingMemoryRegister::Disposer());
	_stagingMemoryRegister.superSizeMemoryAvailableList.clear_and_dispose(StagingMemoryRegister::Disposer());
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
	// (it might be missing during the first call to alloc())
	if(_firstAllocMemory == nullptr) {
		size_t size =
			(numBytes < _smallMemorySize)
				? _smallMemorySize
				: (numBytes < _middleMemorySize)
					? _middleMemorySize
					: max(numBytes, _largeMemorySize);
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
				(numBytes < _middleMemorySize)
					? _middleMemorySize
					: max(numBytes, _largeMemorySize);
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
			size_t size = max(_largeMemorySize, numBytes);
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
	a->stagingFrameNumber = _renderer->frameNumber();
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
	a->stagingFrameNumber = _renderer->frameNumber();

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


tuple<StagingMemory&, bool> DataStorage::allocStagingMemory(DataMemory& m,
	StagingMemory* lastStagingMemory, size_t minNumBytes, size_t bytesToMemoryEnd)
{
	// reuse or create StagingMemory func
	auto reuseOrAllocStagingMemory =
		[](StagingMemoryRegister::StagingMemoryList& availableList,
		   StagingMemoryRegister::StagingMemoryList& inUseList,
		   Renderer& r, size_t size) -> StagingMemory&
		{
			if(!availableList.empty()) {
				auto it = availableList.begin();
				inUseList.splice(inUseList.end(), availableList, it);
				return *it;
			}
			StagingMemory* sm = new StagingMemory(r, size);  // might throw
			inUseList.push_back(*sm);
			return *sm;
		};

	// reuse or create super-size StagingMemory func
	auto reuseOrAllocSuperSizeStagingMemory =
		[](StagingMemoryRegister::StagingMemoryList& availableList,
		   StagingMemoryRegister::StagingMemoryList& inUseList,
		   Renderer& r, size_t size) -> StagingMemory&
		{
			// find first suitable
			for(auto bestIt=availableList.begin(); bestIt!=availableList.end(); bestIt++) {
				if(bestIt->size() >= size) {

					// find smallest of suitable StagingMemories
					auto it = bestIt;
					for(it++; it!=availableList.end(); it++)
						if(it->size() >= size && it->size() < bestIt->size())
							bestIt = it;

					// return best suitable StagingMemory
					inUseList.splice(inUseList.end(), availableList, bestIt);
					return *bestIt;
				}
			}

			// create new StagingMemory
			StagingMemory* sm = new StagingMemory(r, size);  // might throw
			inUseList.push_back(*sm);
			return *sm;
		};

	// we already have staging memory that is probably full =>
	// we are going to allocate another staging memory, but bigger one
	if(lastStagingMemory) {

		// return middle-size memory
		if(m.size() <= _middleMemorySize)
			return {
				reuseOrAllocStagingMemory(
					_stagingMemoryRegister.middleMemoryAvailableList,
					_stagingMemoryRegister.middleMemoryInUseList,
					*_renderer, _middleMemorySize),
				true
			};
		if(lastStagingMemory->size() < _middleMemorySize)
			return {
				reuseOrAllocStagingMemory(
					_stagingMemoryRegister.middleMemoryAvailableList,
					_stagingMemoryRegister.middleMemoryInUseList,
					*_renderer, _middleMemorySize),
				false
			};

		// return large memory
		if(m.size() <= _largeMemorySize)
			return {
				reuseOrAllocStagingMemory(
					_stagingMemoryRegister.largeMemoryAvailableList,
					_stagingMemoryRegister.largeMemoryInUseList,
					*_renderer, _largeMemorySize),
				true
			};
		if(lastStagingMemory->size() < _largeMemorySize)
			return {
				reuseOrAllocStagingMemory(
					_stagingMemoryRegister.largeMemoryAvailableList,
					_stagingMemoryRegister.largeMemoryInUseList,
					*_renderer, _largeMemorySize),
				false
			};

		// return super-size memory
		return {
			reuseOrAllocStagingMemory(
				_stagingMemoryRegister.superSizeMemoryAvailableList,
				_stagingMemoryRegister.superSizeMemoryInUseList,
				*_renderer, m.size()),
			true
		};
	}

	// create small StagingMemory
	if(m.size() <= _smallMemorySize)
		return {
			reuseOrAllocStagingMemory(
				_stagingMemoryRegister.smallMemoryAvailableList,
				_stagingMemoryRegister.smallMemoryInUseList,
				*_renderer, _smallMemorySize),
			true
		};
	if(bytesToMemoryEnd <= _smallMemorySize ||
	   (minNumBytes <= _smallMemorySize && _stagingDataSizeHint*1.2f < _smallMemorySize))
	{
		return {
			reuseOrAllocStagingMemory(
				_stagingMemoryRegister.smallMemoryAvailableList,
				_stagingMemoryRegister.smallMemoryInUseList,
				*_renderer, _smallMemorySize),
			false
		};
	}

	// create middle-sized StagingMemory
	if(m.size() <= _middleMemorySize)
		return {
			reuseOrAllocStagingMemory(
				_stagingMemoryRegister.middleMemoryAvailableList,
				_stagingMemoryRegister.middleMemoryInUseList,
				*_renderer, _middleMemorySize),
			true
		};
	if(bytesToMemoryEnd <= _middleMemorySize ||
	   (minNumBytes <= _middleMemorySize && _stagingDataSizeHint*1.2f < _middleMemorySize))
	{
		return {
			reuseOrAllocStagingMemory(
				_stagingMemoryRegister.middleMemoryAvailableList,
				_stagingMemoryRegister.middleMemoryInUseList,
				*_renderer, _middleMemorySize),
			false
		};
	}

	// large StagingMemory
	if(m.size() <= _largeMemorySize)
		return {
			reuseOrAllocStagingMemory(
				_stagingMemoryRegister.largeMemoryAvailableList,
				_stagingMemoryRegister.largeMemoryInUseList,
				*_renderer, _largeMemorySize),
			true
		};
	if(bytesToMemoryEnd <= _largeMemorySize ||
	   (minNumBytes <= _largeMemorySize && _stagingDataSizeHint*1.2f < _largeMemorySize))
	{
		return {
			reuseOrAllocStagingMemory(
				_stagingMemoryRegister.largeMemoryAvailableList,
				_stagingMemoryRegister.largeMemoryInUseList,
				*_renderer, _largeMemorySize),
			false
		};
	}

	// handle super-size requests
	return {
		reuseOrAllocSuperSizeStagingMemory(
			_stagingMemoryRegister.superSizeMemoryAvailableList,
			_stagingMemoryRegister.superSizeMemoryInUseList,
			*_renderer, minNumBytes),
		true
	};
}


void DataStorage::freeOrRecycleStagingMemory(StagingMemory& sm)
{
	StagingMemoryRegister::StagingMemoryList* srcList;
	StagingMemoryRegister::StagingMemoryList* dstList;
	if(sm.size() <= _smallMemorySize) {
		srcList = &_stagingMemoryRegister.smallMemoryInUseList;
		dstList = &_stagingMemoryRegister.smallMemoryAvailableList;
	}
	else if(sm.size() <= _middleMemorySize) {
		srcList = &_stagingMemoryRegister.middleMemoryInUseList;
		dstList = &_stagingMemoryRegister.middleMemoryAvailableList;
	}
	else if(sm.size() <= _largeMemorySize) {
		srcList = &_stagingMemoryRegister.largeMemoryInUseList;
		dstList = &_stagingMemoryRegister.largeMemoryAvailableList;
	}
	else {
		srcList = &_stagingMemoryRegister.superSizeMemoryInUseList;
		dstList = &_stagingMemoryRegister.superSizeMemoryAvailableList;
	}
	dstList->splice(dstList->begin(), *srcList, srcList->iterator_to(sm));
}


tuple<TransferResourcesReleaser,size_t> DataStorage::recordUploads(vk::CommandBuffer commandBuffer)
{
	// find first valid upload
	size_t dataMemoryIndex = 0;
	size_t dataMemorySize = _dataMemoryList.size();
	DataMemory* dm;
	size_t numBytesToTransfer;
	void* id1;
	void* id2;
	for(; dataMemoryIndex<dataMemorySize; dataMemoryIndex++) {
		dm = _dataMemoryList[dataMemoryIndex];
		tie(id1, id2, numBytesToTransfer) = dm->recordUploads(commandBuffer);
		if(numBytesToTransfer != 0)
			goto foundFirst;
	}

	// return zero bytes transferred 
	return { TransferResourcesReleaser(), 0 };

foundFirst:

	// create Transfer and append first upload operation
	auto it =
		_transferInProgressList.emplace(
			_transferInProgressList.end(),  // position
			dataMemorySize-dataMemoryIndex  // capacity
		);
	TransferResources& tr = *it;
	tr.append(dm, id1, id2);
	dataMemoryIndex++;

	// insert all remaining upload operations
	for(; dataMemoryIndex<dataMemorySize; dataMemoryIndex++) {
		dm = _dataMemoryList[dataMemoryIndex];
		size_t numBytes;
		tie(id1, id2, numBytes) = dm->recordUploads(commandBuffer);
		if(numBytes != 0) {
			// append next upload operation
			tr.append(dm, id1, id2);
			numBytesToTransfer += numBytes;
		}
	}
	return { TransferResourcesReleaser(it, this), numBytesToTransfer };
}


void DataStorage::uploadDone(TransferResourcesReleaser::Id id) noexcept
{
	// verify valid usage
	// (currently, there is no real restriction in our code so we might relax or remove following assert)
	assert(id == _transferInProgressList.begin() && "DataStorage::uploadDone(TransferId) must be called on TransferIds "
	                                                "in the same order as they are returned by DataStorage::recordUpload().");

	// remove transfer from the list
	_transferInProgressList.erase(id);
}
