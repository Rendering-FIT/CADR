#include <CadR/StagingManager.h>
#include <CadR/StagingMemory.h>

using namespace std;
using namespace CadR;



void StagingManager::cleanUp() noexcept
{
	// destroy StagingMemory objects
	_smallMemoryInUseList.clear_and_dispose(Disposer());
	_smallMemoryAvailableList.clear_and_dispose(Disposer());
	_mediumMemoryInUseList.clear_and_dispose(Disposer());
	_mediumMemoryAvailableList.clear_and_dispose(Disposer());
	_largeMemoryInUseList.clear_and_dispose(Disposer());
	_largeMemoryAvailableList.clear_and_dispose(Disposer());
	_superSizeMemoryInUseList.clear_and_dispose(Disposer());
	_superSizeMemoryAvailableList.clear_and_dispose(Disposer());
}


StagingMemory& StagingManager::reuseOrAllocStagingMemory(StagingMemoryList& availableList, StagingMemoryList& inUseList, size_t size)
{
	if(!availableList.empty()) {
		auto it = availableList.begin();
		inUseList.splice(inUseList.end(), availableList, it);
		return *it;
	}
	StagingMemory* sm = new StagingMemory(*this, size);  // might throw
	inUseList.push_back(*sm);
	return *sm;
}


StagingMemory& StagingManager::reuseOrAllocSuperSizeStagingMemory(size_t size)
{
	// find first suitable
	for(auto bestIt=_superSizeMemoryAvailableList.begin(); bestIt!=_superSizeMemoryAvailableList.end(); bestIt++) {
		if(bestIt->size() >= size) {

			// find smallest of suitable StagingMemories
			auto it = bestIt;
			for(it++; it!=_superSizeMemoryAvailableList.end(); it++)
				if(it->size() >= size && it->size() < bestIt->size())
					bestIt = it;

			// return best suitable StagingMemory
			_superSizeMemoryInUseList.splice(_superSizeMemoryInUseList.end(), _superSizeMemoryAvailableList, bestIt);
			return *bestIt;
		}
	}

	// create new StagingMemory
	StagingMemory* sm = new StagingMemory(*this, size);  // might throw
	_superSizeMemoryInUseList.push_back(*sm);
	return *sm;
}


void StagingManager::freeOrRecycleStagingMemory(StagingMemory& sm) noexcept
{
	StagingMemoryList* srcList;
	StagingMemoryList* dstList;
	if(sm.size() <= Renderer::smallMemorySize) {
		srcList = &_smallMemoryInUseList;
		dstList = &_smallMemoryAvailableList;
	}
	else if(sm.size() <= Renderer::mediumMemorySize) {
		srcList = &_mediumMemoryInUseList;
		dstList = &_mediumMemoryAvailableList;
	}
	else if(sm.size() <= Renderer::largeMemorySize) {
		srcList = &_largeMemoryInUseList;
		dstList = &_largeMemoryAvailableList;
	}
	else {
		srcList = &_superSizeMemoryInUseList;
		dstList = &_superSizeMemoryAvailableList;
	}
	dstList->splice(dstList->begin(), *srcList, srcList->iterator_to(sm));  // does not throw
}
