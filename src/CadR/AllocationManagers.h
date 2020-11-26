#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace CadR {

class ItemAllocationManager;
class StagingManager;


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
struct ArrayAllocation {
	uint32_t startIndex;       ///< Index of the start of the allocated array. The real offset is startIndex multiplied by the size of the array item.
	uint32_t numItems;         ///< Number of items in the array. The real size is numItems multiplied by the size of the item.
	uint32_t nextRec;          ///< \brief Index of ArrayAllocation whose allocated memory follows the current block's memory.
	void*    owner;            ///< Object that owns the allocated array. Null indicates free memory block.
	StagingManager* stagingManager;
	ArrayAllocation()  {}  ///< Default constructor. It does nothing.
	ArrayAllocation(uint32_t startIndex,uint32_t numItems,uint32_t nextRec,void* owner);  ///< Constructs object by the given values.
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
class ArrayAllocationManager {
protected:

	std::vector<ArrayAllocation> _allocations;  ///< Array containing allocation info for all allocations. It is indexed by id. Zero id is reserved for invalid id.
	uint32_t _capacity;                    ///< Total number of items (allocated and unallocated).
	uint32_t _available;                   ///< Number of items available for allocation.
	uint32_t _numItemsAvailableAtTheEnd;   ///< Number of available items at the end of the managed memory, e.g. number of items in the block at the end.
	uint32_t _firstItemAvailableAtTheEnd;  ///< Index of the first available item at the end of the managed memory, e.g. the first available item that is followed by available items only.
	uint32_t _idOfArrayAtTheEnd;           ///< Id (index to std::vector\<BlockAllocation\>) of the last allocated block at the end of the managed memory.

public:

	typedef typename std::vector<ArrayAllocation>::iterator iterator;
	typedef typename std::vector<ArrayAllocation>::const_iterator const_iterator;

	static inline constexpr const uint32_t invalidID = 0;

	ArrayAllocationManager(size_t capacity=0);
	ArrayAllocationManager(size_t capacity,uint32_t numItemsOfNullObject);
	void setCapacity(size_t newCapacity);
	size_t capacity() const;                   ///< Returns total number of items (allocated and unallocated).
	size_t available() const;                  ///< Returns the number of items that are available for allocation.
	size_t largestAvailable() const;           ///< Returns the number of available items in the largest continuous array.
	size_t numItemsAvailableAtTheEnd() const;  ///< Returns the number of items that are available at the end of the managed memory, e.g. number of items in the array at the end.
	uint32_t firstItemAvailableAtTheEnd() const; ///< Returns the index of the first available item at the end of the managed memory, e.g. the first available item that is followed by available items only.
	uint32_t idOfArrayAtTheEnd() const;          ///< Returns id (index to std::vector\<ArrayAllocation\>) of the last allocated array at the end of the managed memory.
	uint32_t numNullItems() const;               ///< Returns the number of null items. Null items are stored in the array with Id 0 and always placed on the beginning of the allocated memory or buffer.

	ArrayAllocation& operator[](uint32_t id);
	const ArrayAllocation& operator[](uint32_t id) const;
	size_t numIDs() const;
	iterator begin();
	iterator end();
	const_iterator begin() const;
	const_iterator end() const;

	bool canAllocate(uint32_t numItems) const;
	uint32_t alloc(uint32_t numItems,void* owner);  ///< Allocates number of items. Returns id or zero on failure.
	void free(uint32_t id);  ///< Frees allocated items. Id must be valid. Zero id is allowed and safely ignored.
	void clear();

};


class CADR_EXPORT ItemAllocation {
protected:
	uint32_t _index;
public:

	static inline constexpr const uint32_t invalidID = 0xffffffff;

	constexpr ItemAllocation();
	ItemAllocation(ItemAllocationManager& m);
	ItemAllocation(ItemAllocation&& a,ItemAllocationManager& m);
	~ItemAllocation();

	ItemAllocation(ItemAllocation&&) = delete;  ///< Move-constructor deleted as move operation requires ItemAllocationManager as additional parameter. Use moveFrom() instead.
	ItemAllocation& operator=(ItemAllocation&&) = delete; ///< Move-assignment deleted and replaced by assign() as ItemAllocationManager parameter is required for move operation. Use moveFrom() instead.
	ItemAllocation(const ItemAllocation&) = delete;
	ItemAllocation& operator=(const ItemAllocation&) = delete;

	void moveFrom(ItemAllocation& a,ItemAllocationManager& m);

	constexpr uint32_t index() const;

	uint32_t alloc(ItemAllocationManager& m);
	void free(ItemAllocationManager& m);
	bool isValid() const;

protected:
	constexpr ItemAllocation(uint32_t index);

	friend ItemAllocationManager;
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
 *  tied with cpu memory block, gpu buffers, etc.
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

	ItemAllocationManager(size_t capacity=0);
	ItemAllocationManager(size_t capacity,uint32_t numNullItems);
	~ItemAllocationManager();

	void setCapacity(size_t newCapacity);
	size_t capacity() const;                    ///< Returns total number of items (allocated and unallocated).
	size_t available() const;                   ///< Returns the number of items that are available for allocation.
	size_t numItems() const;
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
	void alloc(ItemAllocation& a);  ///< \brief Allocates one id and stores it in the ItemAllocation. If ItemAllocation has already allocated id, the method does nothing.
	void alloc(ItemAllocation* a,uint32_t numItems);  ///< \brief Allocates number of ids and stores them in the ItemAllocations. As ids are returned from the free pool, they may not be series of consecutive ids. Array pointed by ids must be at least numItems long. ItemAllocations that has already ids allocated are ignored and no allocation for them is performed.
	template<typename It>
	void alloc(It start,It end);  ///< \brief Allocates ids and stores them in range of ItemAllocations given by start and end iterators. As ids are returned from the free pool, they may not be series of consecutive ids. ItemAllocations that has already ids allocated are ignored and no allocation for them is performed.
	void swap(ItemAllocation* a1,ItemAllocation* a2);
	void free(ItemAllocation& a);  ///< Frees allocated item. If item contains invalidID, the method does nothing.
	void free(ItemAllocation* a,uint32_t numItems);  ///< Frees allocated items. The items containing invalidID are safely ignored.
	template<typename It>
	void free(It start,It end);  ///< Frees allocated items given by start and end iterators. The items containing invalidID are safely ignored.
	void clear();
	void assertEmpty();

	static ItemAllocation& nullItem();
};


}



// inline and template methods
#include <cassert>
#include <CadR/Errors.h>
namespace CadR {

inline ArrayAllocation::ArrayAllocation(uint32_t startIndex,uint32_t numItems,uint32_t nextRec,void* owner)
{ this->startIndex=startIndex; this->numItems=numItems; this->nextRec=nextRec; this->owner=owner; this->stagingManager=nullptr; }

inline ArrayAllocationManager::ArrayAllocationManager(size_t capacity)
	: _capacity(uint32_t(capacity)), _available(uint32_t(capacity)), _numItemsAvailableAtTheEnd(uint32_t(capacity)),
	  _firstItemAvailableAtTheEnd(0), _idOfArrayAtTheEnd(0)
{
	// zero element is reserved for invalid object
	// and it serves as Null object (Null object design pattern)
	_allocations.emplace_back(0,0,0,nullptr);
}
inline ArrayAllocationManager::ArrayAllocationManager(size_t capacity,uint32_t numItemsOfNullObject)
	: _capacity(uint32_t(capacity)), _available(uint32_t(capacity-numItemsOfNullObject)),
	  _numItemsAvailableAtTheEnd(uint32_t(capacity-numItemsOfNullObject)),
	  _firstItemAvailableAtTheEnd(numItemsOfNullObject), _idOfArrayAtTheEnd(0)
{
	// zero element is reserved for invalid object
	// and it serves as Null object (Null object design pattern)
	_allocations.emplace_back(0,numItemsOfNullObject,0,nullptr);
}
inline void ArrayAllocationManager::setCapacity(size_t newCapacity)
{
	int32_t delta=int32_t(newCapacity)-_capacity;
	_capacity=uint32_t(newCapacity);
	_available+=delta;
	_numItemsAvailableAtTheEnd+=delta;
}
inline void ArrayAllocationManager::clear()
{
	auto numItemsOfNullArray=numNullItems();
	_allocations.clear();
	_allocations.emplace_back(0,numItemsOfNullArray,0,nullptr);
	_available=_capacity-numItemsOfNullArray;
	_numItemsAvailableAtTheEnd=_available;
	_firstItemAvailableAtTheEnd=numItemsOfNullArray;
	_idOfArrayAtTheEnd=0;
}
inline ItemAllocationManager::ItemAllocationManager(size_t capacity)
	: _pointerList(new ItemAllocation*[capacity]),
	  _capacity(uint32_t(capacity)), _available(uint32_t(capacity)),
	  _firstItemAvailableAtTheEnd(0), _numNullItems(0)  {}
inline ItemAllocationManager::~ItemAllocationManager()
{
	delete[] _pointerList;
}

inline size_t ArrayAllocationManager::capacity() const  { return _capacity; }
inline size_t ArrayAllocationManager::available() const  { return _available; }
inline size_t ArrayAllocationManager::largestAvailable() const  { return _numItemsAvailableAtTheEnd; }
inline size_t ArrayAllocationManager::numItemsAvailableAtTheEnd() const  { return _numItemsAvailableAtTheEnd; }
inline uint32_t ArrayAllocationManager::firstItemAvailableAtTheEnd() const  { return _firstItemAvailableAtTheEnd; }
inline uint32_t ArrayAllocationManager::idOfArrayAtTheEnd() const  { return _idOfArrayAtTheEnd; }
inline uint32_t ArrayAllocationManager::numNullItems() const  { return _allocations[0].numItems; }
inline size_t ItemAllocationManager::capacity() const  { return _capacity; }
inline size_t ItemAllocationManager::available() const  { return _available; }
inline size_t ItemAllocationManager::numItems() const  { return _firstItemAvailableAtTheEnd-_numNullItems; }
inline uint32_t ItemAllocationManager::firstItemAvailableAtTheEnd() const  { return _firstItemAvailableAtTheEnd; }
inline uint32_t ItemAllocationManager::numNullItems() const  { return _numNullItems; }

inline ArrayAllocation& ArrayAllocationManager::operator[](uint32_t id)  { return _allocations[id]; }
inline const ArrayAllocation& ArrayAllocationManager::operator[](uint32_t id) const  { return _allocations[id]; }
inline size_t ArrayAllocationManager::numIDs() const  { return _allocations.size(); }
inline typename ArrayAllocationManager::iterator ArrayAllocationManager::begin()
{ return _allocations.begin()+1; } // there is always null object in the list, so we increment by one to skip it
inline typename ArrayAllocationManager::iterator ArrayAllocationManager::end()
{ return _allocations.end(); }
inline typename ArrayAllocationManager::const_iterator ArrayAllocationManager::begin() const
{ return _allocations.begin()+1; } // there is always null object in the list, so we increment by one to skip it
inline typename ArrayAllocationManager::const_iterator ArrayAllocationManager::end() const
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

inline bool ArrayAllocationManager::canAllocate(uint32_t numItems) const
{ return _numItemsAvailableAtTheEnd>=numItems; }
inline bool ItemAllocationManager::canAllocate(uint32_t numItems) const
{ return _capacity-_firstItemAvailableAtTheEnd>=numItems; }

inline uint32_t ArrayAllocationManager::alloc(uint32_t numItems,void* owner)
{
	if(_numItemsAvailableAtTheEnd<numItems)
		throw CadR::OutOfResources("ArrayAllocationManager::alloc(): Can not allocate Array. Not enough free space.");

	uint32_t id=uint32_t(_allocations.size());
	_allocations.emplace_back(_firstItemAvailableAtTheEnd,numItems,0,owner);
	_allocations.operator[](_idOfArrayAtTheEnd).nextRec=id;
	_available-=numItems;
	_numItemsAvailableAtTheEnd-=numItems;
	_firstItemAvailableAtTheEnd+=numItems;
	_idOfArrayAtTheEnd=id;
	return id;
}
inline void ArrayAllocationManager::free(uint32_t id)  { _allocations.operator[](id).owner=nullptr; }
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
			uint32_t lastItem=_firstItemAvailableAtTheEnd-1;
			if(i!=lastItem) {
				_pointerList[i]=_pointerList[lastItem];
				_pointerList[i]->_index=i;
			}
			_firstItemAvailableAtTheEnd=lastItem;
		}
	}
}
inline void ItemAllocationManager::clear()  { free(begin(),end()); }
inline void ItemAllocationManager::assertEmpty()  { assert(numItems()==0 && "Manager still contains unreleased allocations."); }
inline ItemAllocation& ItemAllocationManager::nullItem()  { return _nullItem; }

inline constexpr uint32_t ItemAllocation::index() const  { return _index; }
inline uint32_t ItemAllocation::alloc(ItemAllocationManager& m)  { m.alloc(*this); return _index; }
inline void ItemAllocation::free(ItemAllocationManager& m)  { m.free(*this); }
inline bool ItemAllocation::isValid() const  { return _index!=ItemAllocation::invalidID; }

inline constexpr ItemAllocation::ItemAllocation() : _index(ItemAllocation::invalidID)  {}
inline ItemAllocation::ItemAllocation(ItemAllocationManager& m) : _index(ItemAllocation::invalidID)  { m.alloc(*this); }
inline ItemAllocation::ItemAllocation(ItemAllocation&& a,ItemAllocationManager& m) : _index(a._index)  { if(a._index==ItemAllocation::invalidID) return; a._index=ItemAllocation::invalidID; m[_index]=this; }
inline ItemAllocation::~ItemAllocation()  { assert((_index==ItemAllocation::invalidID || this==&ItemAllocationManager::nullItem()) && "Item is still allocated! Always free items before their destruction! Destructor cannot do it for you as it has no access to ItemAllocationManager."); }

inline void ItemAllocation::moveFrom(ItemAllocation& a,ItemAllocationManager& m)
{
	m.free(*this);
	_index=a._index;
	if(a._index!=ItemAllocation::invalidID) {
		a._index=ItemAllocation::invalidID;
		m[_index]=this;
	}
}

inline constexpr ItemAllocation::ItemAllocation(uint32_t index) : _index(index)  {}

}
