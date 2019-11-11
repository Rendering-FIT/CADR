#include <CadR/AllocationManagers.h>
#include <cstring>

using namespace CadR;

static_assert(sizeof(ItemAllocation)==4, "CadR::ItemAllocation class must take exactly 4 bytes. Otherwise consider redesign of ItemAllocationManager.");

ItemAllocation ItemAllocationManager::_nullItem=0;

static struct NullItemFinalizer {
	~NullItemFinalizer() {
		assert(ItemAllocationManager::nullItem().index()==0 && "_nullItemID was changed. It is expected to remain zero throughout application life time. "
		                       "   You probably accessed or reallocated a null item.");
	}
} nullItemFinalizer;



ItemAllocationManager::ItemAllocationManager(uint32_t capacity,uint32_t numNullItems)
	: _pointerList(new ItemAllocation*[capacity]),
	  _capacity(capacity), _available(capacity-numNullItems),
	  _firstItemAvailableAtTheEnd(numNullItems), _numNullItems(numNullItems)
{
	assert(capacity<numNullItems && "No space for null items.");
	for(uint32_t i=0; i<numNullItems; i++)
		_pointerList[i]=&_nullItem;
}


void ItemAllocationManager::setCapacity(uint32_t newCapacity)
{
	if(newCapacity==_capacity)
		return;
	assert(_firstItemAvailableAtTheEnd>newCapacity && "ItemAllocationManager::setCapacity(): Attempt to reduce capacity bellow already allocated space.");

	ItemAllocation** newPointerList=new ItemAllocation*[newCapacity];
	try {
		memcpy(newPointerList,_pointerList,_firstItemAvailableAtTheEnd*sizeof(ItemAllocation*));
	} catch(...) {
		delete[] newPointerList;
		throw;
	}
	delete[] _pointerList;
	_pointerList=newPointerList;
	_capacity=newCapacity;
}


void ItemAllocationManager::alloc(ItemAllocation* a)
{
	if(a->_index!=invalidID)
		return;

	if(_capacity-_firstItemAvailableAtTheEnd<1)
		throw CadR::OutOfResources("ItemAllocationManager::alloc(): Can not allocate item. Not enough free space.");

	a->_index=_firstItemAvailableAtTheEnd;
	_pointerList[_firstItemAvailableAtTheEnd]=a;
	_firstItemAvailableAtTheEnd++;
}


void ItemAllocationManager::alloc(ItemAllocation* a,uint32_t numItems)
{
	if(_capacity-_firstItemAvailableAtTheEnd<numItems)
		throw CadR::OutOfResources("ItemAllocationManager::alloc(): Can not allocate items. Not enough free space.");

	for(uint32_t i=0; i<numItems; i++) {
		if(a->_index!=invalidID)
			continue;
		a[i]._index=_firstItemAvailableAtTheEnd;
		_pointerList[_firstItemAvailableAtTheEnd]=&a[i];
		_firstItemAvailableAtTheEnd++;
	}
}


void ItemAllocationManager::free(ItemAllocation* a)
{
	uint32_t i=a->_index;
	if(i!=invalidID) {
		a->_index=invalidID;
		if(i!=_firstItemAvailableAtTheEnd-1) {
			_pointerList[i]=_pointerList[_firstItemAvailableAtTheEnd-1];
			_pointerList[i]->_index=i;
			_firstItemAvailableAtTheEnd--;
		}
	}
}


void ItemAllocationManager::free(ItemAllocation* a,uint32_t numItems)
{
	for(uint32_t j=0; j<numItems; j++) {
		uint32_t i=a[j]._index;
		if(i!=invalidID) {
			a[j]._index=invalidID;
			if(i!=_firstItemAvailableAtTheEnd-1) {
				_pointerList[i]=_pointerList[_firstItemAvailableAtTheEnd-1];
				_pointerList[i]->_index=i;
				_firstItemAvailableAtTheEnd--;
			}
		}
	}
}
