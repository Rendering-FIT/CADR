#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <CadR/Export.h>

namespace CadR {

class ItemAllocationManager;


/** ArrayAllocation represents a single allocation of memory.
 *  It is used by ArrayAllocationManager to facilitate
 *  allocations of item arrays of various sizes.
 *  The allocation granularity is size of one item.
 *
 *  ArrayAllocationManager keeps vector of ArrayAllocations accessible by
 *  integer-based ID value. Each ID value represents single memory allocation.
 *  The ID value is returned to the user during the allocation process.
 *
 *  The smallest allocation is one array element, or theoretically zero
 *  for allocating just ID. All the items in the array are expected to be
 *  of the same size. The smallest item size is 1 byte.
 */
template<typename Owner>
struct ArrayAllocation {
	uint32_t startIndex;       ///< Index of the start of the allocated array. The real offset is startIndex multiplied by the size of the array item.
	uint32_t numItems;         ///< Number of items in the array. The real size is numItems multiplied by the size of the item.
	uint32_t nextRec;          ///< \brief Index of ArrayAllocation whose allocated memory follows the current block's memory.
	Owner*   owner;            ///< Object that owns the allocated array. Null indicates free memory block.
	ArrayAllocation()  {}  ///< Default constructor. It does nothing.
	ArrayAllocation(uint32_t startIndex,uint32_t numItems,uint32_t nextRec,Owner* owner);  ///< Constructs object by the given values.
};


/** ArrayAllocationManager provides memory management functionality
 *  for item arrays of various sizes. Number of items in each allocation
 *  is requested by the user during the allocation process.
 *  Continuous memory block is allocated and its ID is returned to the user.
 *
 *  ArrayAllocationManager template provides only allocation management.
 *  It does not maintain any memory buffer for storing the items.
 *  See BufferStorage template for combining VkBuffer and
 *  ArrayAllocationManager functionality.
 *
 *  Allocation granularity is size of a single item.
 *  ArrayAllocationManager expect all items to be of the same size,
 *  for example 4 bytes for float buffer, or 64 for buffer of 4x4 float matrices.
 *  In general, the item can be anything from a single byte to a very large
 *  structure or binary blob.
 *
 *  Each memory block is identified by the integer-based ID,
 *  starting from 1. ID zero is reserved for non-valid ID.
 *  The smallest allocation unit is one item, or 0
 *  just for allocation of ID. The largest allocation
 *  is equal to the largest available continuous block.
 */
template<typename Owner>
class ArrayAllocationManager {
protected:

	std::vector<ArrayAllocation<Owner>> _allocations;  ///< Array containing allocation info for all allocations. It is indexed by id. Zero id is reserved for invalid id.
	uint32_t _capacity;                    ///< Total number of items (allocated and unallocated).
	uint32_t _available;                   ///< Number of items available for allocation.
	uint32_t _numItemsAvailableAtTheEnd;   ///< Number of available items at the end of the managed memory, e.g. number of items in the block at the end.
	uint32_t _firstItemAvailableAtTheEnd;  ///< Index of the first available item at the end of the managed memory, e.g. the first available item that is followed by available items only.
	uint32_t _idOfArrayAtTheEnd;           ///< Id (index to std::vector\<BlockAllocation\>) of the last allocated block at the end of the managed memory.

public:

	typedef Owner AllocationOwner;
	typedef typename std::vector<ArrayAllocation<Owner>>::iterator iterator;
	typedef typename std::vector<ArrayAllocation<Owner>>::const_iterator const_iterator;

	ArrayAllocationManager(uint32_t capacity=0);
	ArrayAllocationManager(uint32_t capacity,uint32_t numItemsOfNullObject);
	void setCapacity(uint32_t newCapacity);
	uint32_t capacity() const;                   ///< Returns total number of items (allocated and unallocated).
	uint32_t available() const;                  ///< Returns the number of items that are available for allocation.
	uint32_t largestAvailable() const;           ///< Returns the number of available items in the largest continuous array.
	uint32_t numItemsAvailableAtTheEnd() const;  ///< Returns the number of items that are available at the end of the managed memory, e.g. number of items in the array at the end.
	uint32_t firstItemAvailableAtTheEnd() const; ///< Returns the index of the first available item at the end of the managed memory, e.g. the first available item that is followed by available items only.
	uint32_t idOfArrayAtTheEnd() const;          ///< Returns id (index to std::vector\<ArrayAllocation\>) of the last allocated array at the end of the managed memory.
	uint32_t numNullItems() const;               ///< Returns the number of null items. Null items are stored in the array with Id 0 and always placed on the beginning of the allocated memory or buffer.

	ArrayAllocation<Owner>& operator[](uint32_t id);
	const ArrayAllocation<Owner>& operator[](uint32_t id) const;
	size_t numIDs() const;
	iterator begin();
	iterator end();
	const_iterator begin() const;
	const_iterator end() const;

	bool canAllocate(uint32_t numItems) const;
	uint32_t alloc(uint32_t numItems,Owner* owner);  ///< Allocates number of items. Returns id or zero on failure.
	void free(uint32_t id);  ///< Frees allocated items. Id must be valid. Zero id is allowed and safely ignored.
	void clear();

};


class CADR_EXPORT ItemAllocation {
protected:
	uint32_t _index;
	friend ItemAllocationManager;
public:

	constexpr uint32_t index() const;
	uint32_t alloc(ItemAllocationManager& m);
	void free(ItemAllocationManager& m);

	constexpr ItemAllocation();
	ItemAllocation(ItemAllocationManager& m);
	ItemAllocation(ItemAllocation&& a,ItemAllocationManager& m);
	~ItemAllocation();

	void assign(ItemAllocation&& a,ItemAllocationManager& m);

	ItemAllocation(ItemAllocation&&) = delete;
	ItemAllocation(const ItemAllocation&) = delete;
	ItemAllocation& operator=(ItemAllocation&&) = delete;
	ItemAllocation& operator=(const ItemAllocation&) = delete;

protected:
	constexpr ItemAllocation(uint32_t index);
};


/** ItemAllocationManager provides memory management functionality for
 *  allocating single items. Each allocated item is identified by
 *  integer-based ID, begining at 0 for the first item
 *  and ending at numItemsTotal()-1 for the last item.
 *
 *  All the items must be of the same size,
 *  but the item can be anything from a single byte to a large structure.
 *
 *  Two allocation methods are provided. One allocates single item and returns
 *  its ID. The second allocates multiple items and returns array of IDs allocated.
 *  The largest allocation possible is equal to the number of available items.
 *
 *  ItemAllocationManager class does not maintain any memory buffer.
 *  It provides memory management functionality that could be
 *  tied with cpu memory block, gpu buffers, etc. See BufferStorage template
 *  for combining ge::gl::BufferObject and ItemAllocationManager functionality.
 */
class CADR_EXPORT ItemAllocationManager {
protected:

	ItemAllocation** _pointerList;
	uint32_t _capacity;                    ///< Total number of items (allocated plus unallocated).
	uint32_t _available;                   ///< Number of items available for allocation.
	uint32_t _firstItemAvailableAtTheEnd;  ///< Index of the first available item at the end of the managed memory that is followed by available items only.
	uint32_t _numNullItems;                ///< Number of null objects (null design pattern) allocated at the beginning of the managed memory.
	static ItemAllocation _nullItem;

public:

	typedef std::array<ItemAllocation*,0>::iterator iterator;
	typedef std::array<ItemAllocation*,0>::const_iterator const_iterator;
	static inline constexpr const uint32_t invalidID = 0xffffffff;

	ItemAllocationManager(uint32_t capacity=0);
	ItemAllocationManager(uint32_t capacity,uint32_t numNullItems);
	~ItemAllocationManager();

	void setCapacity(uint32_t newCapacity);
	uint32_t capacity() const;                    ///< Returns total number of items (allocated and unallocated).
	uint32_t available() const;                   ///< Returns the number of items that are available for allocation.
	uint32_t numItems() const;
	uint32_t firstItemAvailableAtTheEnd() const;  ///< Returns the index of the first available item at the end of the managed memory, e.g. the first available item that is followed by available items only.
	uint32_t numNullItems() const;                ///< Returns the number of null items (null design pattern) allocated at the beginning of the managed memory. Ids of null items are in the range 0 and numNullItems()-1.

	ItemAllocation*& operator[](uint32_t id);
	ItemAllocation*const& operator[](uint32_t id) const;
	ItemAllocation* allocation(uint32_t id) const;
	iterator begin();
	iterator end();
	const_iterator begin() const;
	const_iterator end() const;
	uint32_t firstID() const;
	uint32_t lastID() const;

	bool canAllocate(uint32_t numItems) const;
	void alloc(ItemAllocation* a);  ///< \brief Allocates one item and stores the item's id in the variable pointed by id parameter.
	void alloc(ItemAllocation* a,uint32_t numItems);  ///< \brief Allocates number of items. The returned items may not form the continuous block of memory. Array pointed by ids must be at least numItems long.
	template<typename It>
	void alloc(It start,It end);  ///< \brief Allocates items in the range given by start and end iterators. The returned items may not form the continuous block of memory.
	void swap(ItemAllocation* a1,ItemAllocation* a2);
	void free(ItemAllocation* a);  ///< Frees allocated item. Id must be valid.
	void free(ItemAllocation* a,uint32_t numItems);  ///< Frees allocated items. Ids pointed by ids parameter must be valid.
	template<typename It>
	void free(It start,It end);  ///< Frees allocated items given by start and end iterators. Ids pointed by ids parameter must be valid.
	void clear();
	void assertEmpty();

	static ItemAllocation& nullItem();
};


}



// inline and template methods
#include <cassert>
#include <CadR/Errors.h>
namespace CadR {

template<typename Owner> inline ArrayAllocation<Owner>::ArrayAllocation(uint32_t startIndex,uint32_t numItems,uint32_t nextRec,Owner* owner)
{ this->startIndex=startIndex; this->numItems=numItems; this->nextRec=nextRec; this->owner=owner; }

template<typename Owner>
inline ArrayAllocationManager<Owner>::ArrayAllocationManager(uint32_t capacity)
	: _capacity(capacity), _available(capacity), _numItemsAvailableAtTheEnd(capacity),
	  _firstItemAvailableAtTheEnd(0), _idOfArrayAtTheEnd(0)
{
	// zero element is reserved for invalid object
	// and it serves as Null object (Null object design pattern)
	_allocations.emplace_back(0,0,0,nullptr);
}
template<typename Owner>
inline ArrayAllocationManager<Owner>::ArrayAllocationManager(uint32_t capacity,uint32_t numItemsOfNullObject)
	: _capacity(capacity), _available(capacity-numItemsOfNullObject),
	  _numItemsAvailableAtTheEnd(capacity-numItemsOfNullObject),
	  _firstItemAvailableAtTheEnd(numItemsOfNullObject), _idOfArrayAtTheEnd(0)
{
	// zero element is reserved for invalid object
	// and it serves as Null object (Null object design pattern)
	_allocations.emplace_back(0,numItemsOfNullObject,0,nullptr);
}
template<typename Owner> void ArrayAllocationManager<Owner>::setCapacity(uint32_t newCapacity)
{
	int delta=newCapacity-_capacity;
	_capacity=newCapacity;
	_available+=delta;
	_numItemsAvailableAtTheEnd+=delta;
}
template<typename Owner>
void ArrayAllocationManager<Owner>::clear()
{
	auto numItemsOfNullArray=numNullItems();
	_allocations.clear();
	_allocations.emplace_back(0,numItemsOfNullArray,0,nullptr);
	_available=_capacity-numItemsOfNullArray;
	_numItemsAvailableAtTheEnd=_available;
	_firstItemAvailableAtTheEnd=numItemsOfNullArray;
	_idOfArrayAtTheEnd=0;
}
inline ItemAllocationManager::ItemAllocationManager(uint32_t capacity)
	: _pointerList(new ItemAllocation*[capacity]),
	  _capacity(capacity), _available(capacity),
	  _firstItemAvailableAtTheEnd(0), _numNullItems(0)  {}
inline ItemAllocationManager::~ItemAllocationManager()
{
	delete[] _pointerList;
}

template<typename Owner> inline uint32_t ArrayAllocationManager<Owner>::capacity() const  { return _capacity; }
template<typename Owner> inline uint32_t ArrayAllocationManager<Owner>::available() const  { return _available; }
template<typename Owner> inline uint32_t ArrayAllocationManager<Owner>::largestAvailable() const  { return _numItemsAvailableAtTheEnd; }
template<typename Owner> inline uint32_t ArrayAllocationManager<Owner>::numItemsAvailableAtTheEnd() const  { return _numItemsAvailableAtTheEnd; }
template<typename Owner> inline uint32_t ArrayAllocationManager<Owner>::firstItemAvailableAtTheEnd() const  { return _firstItemAvailableAtTheEnd; }
template<typename Owner> inline uint32_t ArrayAllocationManager<Owner>::idOfArrayAtTheEnd() const  { return _idOfArrayAtTheEnd; }
template<typename Owner> inline uint32_t ArrayAllocationManager<Owner>::numNullItems() const  { return _allocations[0].numItems; }
inline uint32_t ItemAllocationManager::capacity() const  { return _capacity; }
inline uint32_t ItemAllocationManager::available() const  { return _available; }
inline uint32_t ItemAllocationManager::numItems() const  { return _firstItemAvailableAtTheEnd-_numNullItems; }
inline uint32_t ItemAllocationManager::firstItemAvailableAtTheEnd() const  { return _firstItemAvailableAtTheEnd; }
inline uint32_t ItemAllocationManager::numNullItems() const  { return _numNullItems; }

template<typename Owner> inline ArrayAllocation<Owner>& ArrayAllocationManager<Owner>::operator[](uint32_t id)  { return _allocations[id]; }
template<typename Owner> inline const ArrayAllocation<Owner>& ArrayAllocationManager<Owner>::operator[](uint32_t id) const  { return _allocations[id]; }
template<typename Owner> inline size_t ArrayAllocationManager<Owner>::numIDs() const  { return _allocations.size(); }
template<typename Owner> inline typename ArrayAllocationManager<Owner>::iterator ArrayAllocationManager<Owner>::begin()
{ return _allocations.begin()+1; } // there is always null object in the list, so we increment by one to skip it
template<typename Owner> inline typename ArrayAllocationManager<Owner>::iterator ArrayAllocationManager<Owner>::end()
{ return _allocations.end(); }
template<typename Owner> inline typename ArrayAllocationManager<Owner>::const_iterator ArrayAllocationManager<Owner>::begin() const
{ return _allocations.begin()+1; } // there is always null object in the list, so we increment by one to skip it
template<typename Owner> inline typename ArrayAllocationManager<Owner>::const_iterator ArrayAllocationManager<Owner>::end() const
{ return _allocations.end(); }
inline ItemAllocation*& ItemAllocationManager::operator[](uint32_t id)  { return _pointerList[id]; }
inline ItemAllocation*const& ItemAllocationManager::operator[](uint32_t id) const  { return _pointerList[id]; }
inline ItemAllocation* ItemAllocationManager::allocation(uint32_t id) const  { return _pointerList[id]; }
inline ItemAllocationManager::iterator ItemAllocationManager::begin()  { return reinterpret_cast<std::array<ItemAllocation*,0>*>(_pointerList)->begin()+_numNullItems; }
inline ItemAllocationManager::iterator ItemAllocationManager::end()  { return reinterpret_cast<std::array<ItemAllocation*,0>*>(_pointerList)->begin()+_firstItemAvailableAtTheEnd; }
inline ItemAllocationManager::const_iterator ItemAllocationManager::begin() const  { return reinterpret_cast<std::array<ItemAllocation*,0>*>(_pointerList)->begin()+_numNullItems; }
inline ItemAllocationManager::const_iterator ItemAllocationManager::end() const  { return reinterpret_cast<std::array<ItemAllocation*,0>*>(_pointerList)->begin()+_firstItemAvailableAtTheEnd; }
inline uint32_t ItemAllocationManager::firstID() const  { return _numNullItems; }
inline uint32_t ItemAllocationManager::lastID() const  { return _firstItemAvailableAtTheEnd-1; }

template<typename Owner> inline bool ArrayAllocationManager<Owner>::canAllocate(uint32_t numItems) const
{ return _numItemsAvailableAtTheEnd>=numItems; }
inline bool ItemAllocationManager::canAllocate(uint32_t numItems) const
{ return _capacity-_firstItemAvailableAtTheEnd>=numItems; }

template<typename Owner>
uint32_t ArrayAllocationManager<Owner>::alloc(uint32_t numItems,Owner* owner)
{
	if(_numItemsAvailableAtTheEnd<numItems)
		throw CadR::OutOfResources("ArrayAllocationManager<Owner>::alloc(): Can not allocate Array. Not enough free space.");

	uint32_t id=uint32_t(_allocations.size());
	_allocations.emplace_back(_firstItemAvailableAtTheEnd,numItems,0,owner);
	_allocations.operator[](_idOfArrayAtTheEnd).nextRec=id;
	_available-=numItems;
	_numItemsAvailableAtTheEnd-=numItems;
	_firstItemAvailableAtTheEnd+=numItems;
	_idOfArrayAtTheEnd=id;
	return id;
}
template<typename Owner> inline void ArrayAllocationManager<Owner>::free(uint32_t id)  { _allocations.operator[](id).owner=nullptr; }
template<typename It>
void ItemAllocationManager::alloc(It start,It end)
{
	uint32_t numItems=end-start;
	if(_capacity-_firstItemAvailableAtTheEnd<numItems)
		throw CadR::OutOfResources("ItemAllocationManager::alloc(): Can not allocate items. Not enough free space.");

	for(auto it=start; it!=end; it++) {
		if((*it)->_index!=invalidID)
			continue;
		(*it)->_index=_firstItemAvailableAtTheEnd;
		_pointerList[_firstItemAvailableAtTheEnd]=it;
		_firstItemAvailableAtTheEnd++;
	}
}
inline void ItemAllocationManager::swap(ItemAllocation* a1,ItemAllocation* a2)
{
	uint32_t i1=a1->_index;
	if(i1!=invalidID)
		operator[](i1)=a2;
	uint32_t i2=a2->_index;
	if(i2!=invalidID)
		operator[](i2)=a1;
	a1->_index=i2;
	a2->_index=i1;
}
template<typename It>
void ItemAllocationManager::free(It start,It end)
{
	for(auto it=start; it!=end; it++) {
		uint32_t i=(*it)->_index;
		if(i!=invalidID) {
			(*it)->_index=invalidID;
			if(i!=_firstItemAvailableAtTheEnd-1) {
				_pointerList[i]=_pointerList[_firstItemAvailableAtTheEnd-1];
				_pointerList[i]->_index=i;
				_firstItemAvailableAtTheEnd--;
			}
		}
	}
}
inline void ItemAllocationManager::clear()  { free(begin(),end()); }
inline void ItemAllocationManager::assertEmpty()  { assert(numItems()==0 && "Manager still contains unreleased allocations."); }
inline ItemAllocation& ItemAllocationManager::nullItem()  { return _nullItem; }

inline constexpr uint32_t ItemAllocation::index() const  { return _index; }
inline uint32_t ItemAllocation::alloc(ItemAllocationManager& m)  { m.alloc(this); return _index; }
inline void ItemAllocation::free(ItemAllocationManager& m)  { m.free(this); }

inline constexpr ItemAllocation::ItemAllocation() : _index(ItemAllocationManager::invalidID)  {}
inline ItemAllocation::ItemAllocation(ItemAllocationManager& m) : _index(ItemAllocationManager::invalidID)  { m.alloc(this); }
inline ItemAllocation::ItemAllocation(ItemAllocation&& a,ItemAllocationManager& m) : _index(a._index)  { if(a._index==ItemAllocationManager::invalidID) return; a._index=ItemAllocationManager::invalidID; m[_index]=this; }
inline ItemAllocation::~ItemAllocation()  { assert(_index==ItemAllocationManager::invalidID && "Item is still allocated! Always free items before their destruction!"); }

inline void ItemAllocation::assign(ItemAllocation&& a,ItemAllocationManager& m)
{
	m.free(this);
	_index=a._index;
	if(a._index!=ItemAllocationManager::invalidID) {
		a._index=ItemAllocationManager::invalidID;
		m[_index]=this;
	}
}

inline constexpr ItemAllocation::ItemAllocation(uint32_t index) : _index(index)  {}

}
