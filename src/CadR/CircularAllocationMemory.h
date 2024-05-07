#pragma once

#include <boost/intrusive/list.hpp>
#include <array>
#include <cassert>
#include <cstdint>

namespace CadR {


/** CircularAllocationMemory provides common functionality for efficient suballocation.
 *  It is expected to be subclassed. The derived class shall provide concrete buffer and programmer interface.
 *
 *  AllocationRecord template parameter shall be a struct or a class that holds allocation data details.
 *  The allocation data can be used by the inherited class while there are some requirements and restrictions.
 *  For maximum efficiency, AllocationRecord is used also by CircularAllocationMemory,
 *  especially when the particular AllocationRecord is in the freed state.
 *  For this reason, AllocationRecord needs to be atleast 32 bytes in size.
 *  When in freed state, its content is in complete control of CircularAllocationMemory.
 *  When it is in allocated state, the first two members have certain restrictions. 
 *  The first member shall be uint64_t or pointer type indicating the offset
 *  or memory address of allocated object. The user shall not change its value himself.
 *  It might become 0 or nullptr when particular allocation is of zero size.
 *  The second member shall be any 8-byte variable, such as size_t, representing size of
 *  the allocated object or pointer to the owner object.
 *  There are three reserved values for this member: UINT64_MAX, UINT64_MAX-1 and UINT64_MAX-2.
 *  These values are internally used by CircularAllocationMemory for the management of freed allocation objects.
 *  These three values were chosen because it shall theoretically not disturb size or pointer values
 *  stored in the second member. If size is stored there, values of UINT64_MAX would mean size of
 *  18 Exabytes which is not expected to be used by standard applications.
 *  If the pointer is stored there, the value of UINT64_MAX, possibly decreased by 1 or 2,
 *  would mean pointer to an object that takes one, two or three bytes just at the end of address space.
 *  It is almost non-sence to have an owner object of one, two or three bytes in size placed at the very end of address space.
 */
template<typename AllocationRecord, size_t RecordsPerBlock = 200>
class CircularAllocationMemory {
protected:

	// memory pointers, etc.
	uint64_t _usedBlock2EndAddress;
	uint64_t _usedBlock2StartAddress;
	uint64_t _usedBlock1EndAddress;
	uint64_t _usedBlock1StartAddress;
	uint64_t _bufferEndAddress;
	uint64_t _bufferStartAddress;
	size_t _usedBytes = 0;  ///< Amount of allocated memory. It includes padding used for allocation alignment.

	// AllocationBlock related types
	struct AllocationBlock : boost::intrusive::list_base_hook<
		                         boost::intrusive::link_mode<boost::intrusive::auto_unlink>>
	{
		std::array<AllocationRecord, RecordsPerBlock+2> allocations;
	};
	using AllocationBlockIterator = typename std::array<AllocationRecord, RecordsPerBlock+2>::iterator;
	using AllocationBlockList =
		boost::intrusive::list<
			AllocationBlock,
			boost::intrusive::link_mode<boost::intrusive::auto_unlink>,
			boost::intrusive::constant_time_size<false>>;

	// AllocationBlockLists and iterators 
	AllocationBlockList _allocationBlockList2;
	AllocationBlockIterator _usedBlock2EndAllocation;  //< It points after the last returned allocation from UsedBlock2.
	AllocationBlockList _allocationBlockList1;
	AllocationBlockIterator _usedBlock1EndAllocation;  //< It points after the last returned allocation from UsedBlock1.
	AllocationBlockList _allocationBlockRecycleList;
	
	// SpecialAllocation used for non-allocated items
	struct SpecialAllocation {
		uint64_t address;  ///< Address of allocated memory. Even if the allocation is freed, address still contains the address that used to be assigned to this allocation. 
		uint64_t magicValue;  ///< If the allocation is invalid, it contains special value of UINT64_MAX.
		uint32_t index;  ///< Index of SpecialAllocation within AllocationBlock, starting from 0.
		CircularAllocationMemory<AllocationRecord, RecordsPerBlock>* circularAllocationMemory;
	};
	struct LastAllocation;
	struct FirstAllocation {
		LastAllocation* prevAllocation;
		uint64_t magicValue;  ///< It contains special value of UINT64_MAX-1.
		uint32_t index;  ///< Index of SpecialAllocation within AllocationBlock, starting from 0.
		CircularAllocationMemory<AllocationRecord, RecordsPerBlock>* circularAllocationMemory;
	};
	struct LastAllocation {
		uint64_t address;  ///< Address of allocated memory of the following allocation in the next AllocationBlock, or null if the next AllocationBlock was not created yet. 
		uint64_t magicValue;  ///< It contains special value of UINT64_MAX-1.
		uint32_t index;  ///< Index of SpecialAllocation within AllocationBlock, starting from 0.
		FirstAllocation* nextAllocation;
	};

	// basic allocation/deallocation functions
	AllocationRecord* createAllocation2(uint64_t addr);
	AllocationRecord* createAllocation1(uint64_t addr);
	static void destroyAllocation(AllocationRecord* a) noexcept;
	AllocationBlock& createAllocationBlock();
	void destroyAllocationBlock(AllocationBlock& b) noexcept;
	static void resetDataMemoryPointers(uint64_t aThisAddress,
		CircularAllocationMemory<AllocationRecord, RecordsPerBlock>* m) noexcept;
	template<typename... Args>
		AllocationRecord* allocInternal(size_t numBytes, Args... args);
	void freeInternal(AllocationRecord* a) noexcept;

public:

	~CircularAllocationMemory();

};


template<typename AllocationRecord, size_t RecordsPerBlock>
CircularAllocationMemory<AllocationRecord, RecordsPerBlock>::~CircularAllocationMemory()
{
	// release AllocationBlock memory
	_allocationBlockList2.clear_and_dispose([](AllocationBlock* b){ delete b; });
	_allocationBlockList1.clear_and_dispose([](AllocationBlock* b){ delete b; });
	_allocationBlockRecycleList.clear_and_dispose([](AllocationBlock* b){ delete b; });
}


template<typename AllocationRecord, size_t RecordsPerBlock>
AllocationRecord* CircularAllocationMemory<AllocationRecord, RecordsPerBlock>::createAllocation2(uint64_t addr)
{
	AllocationRecord* a;
	if(_allocationBlockList2.empty())
	{
		// no allocation blocks yet
		AllocationBlock& bNew = createAllocationBlock();
		_allocationBlockList2.push_back(bNew);
		_usedBlock2EndAllocation = bNew.allocations.begin();
		_usedBlock2EndAllocation++;
		a = &(*_usedBlock2EndAllocation);
		_usedBlock2EndAllocation++;

		// write special allocation after the current allocation to serve as stopper for some algorithms
		SpecialAllocation* next = reinterpret_cast<SpecialAllocation*>(&(*_usedBlock2EndAllocation));
		next->address = _usedBlock2EndAddress;
		next->magicValue = UINT64_MAX-2;
	}
	else {
		auto eIt = _allocationBlockList2.back().allocations.end();
		if(_usedBlock2EndAllocation < eIt-2)
		{
			// allocating "inside" AllocationBlock
			a = &(*_usedBlock2EndAllocation);
			_usedBlock2EndAllocation++;

			// write special allocation after the current allocation to serve as stopper for some algorithms
			SpecialAllocation* next = reinterpret_cast<SpecialAllocation*>(&(*_usedBlock2EndAllocation));
			next->address = _usedBlock2EndAddress;
			next->magicValue = UINT64_MAX-2;
		}
		else if(_usedBlock2EndAllocation == eIt-2)
		{
			// allocating the very last item before the ending one
			a = &(*_usedBlock2EndAllocation);
			_usedBlock2EndAllocation++;
		}
		else
		{
			// no more space in the AllocationBlock =>
			// allocate new one
			AllocationBlock& bPrev = _allocationBlockList2.back();
			AllocationBlock& bNew = createAllocationBlock();
			_allocationBlockList2.push_back(bNew);
			_usedBlock2EndAllocation = bNew.allocations.begin();
			_usedBlock2EndAllocation++;
			a = &(*_usedBlock2EndAllocation);
			_usedBlock2EndAllocation++;

			// link new and previous AllocationBlock
			LastAllocation& lPrev = reinterpret_cast<LastAllocation&>(bPrev.allocations.back());
			lPrev.address = addr;
			lPrev.nextAllocation = reinterpret_cast<FirstAllocation*>(&bNew.allocations.front());
			reinterpret_cast<FirstAllocation&>(bNew.allocations.front()).prevAllocation = &lPrev;

			// write special allocation after the current allocation to serve as stopper for some algorithms
			SpecialAllocation* next = reinterpret_cast<SpecialAllocation*>(&(*_usedBlock2EndAllocation));
			next->address = _usedBlock2EndAddress;
			next->magicValue = UINT64_MAX-2;
		}
	}

	// return the allocation
	return a;
}


template<typename AllocationRecord, size_t RecordsPerBlock>
AllocationRecord* CircularAllocationMemory<AllocationRecord, RecordsPerBlock>::createAllocation1(uint64_t addr)
{
	AllocationRecord* a;
	if(_allocationBlockList1.empty())
	{
		// no allocation blocks yet
		AllocationBlock& bNew = createAllocationBlock();
		_allocationBlockList1.push_back(bNew);
		_usedBlock1EndAllocation = bNew.allocations.begin();
		_usedBlock1EndAllocation++;
		a = &(*_usedBlock1EndAllocation);
		_usedBlock1EndAllocation++;

		// write special allocation after the current allocation to serve as stopper for some algorithms
		SpecialAllocation* next = reinterpret_cast<SpecialAllocation*>(&(*_usedBlock1EndAllocation));
		next->address = _usedBlock1EndAddress;
		next->magicValue = UINT64_MAX-2;
	}
	else {
		auto eIt = _allocationBlockList1.back().allocations.end();
		if(_usedBlock1EndAllocation < eIt-2)
		{
			// allocating "inside" AllocationBlock
			a = &(*_usedBlock1EndAllocation);
			_usedBlock1EndAllocation++;

			// write special allocation after the current allocation to serve as stopper for some algorithms
			SpecialAllocation* next = reinterpret_cast<SpecialAllocation*>(&(*_usedBlock1EndAllocation));
			next->address = _usedBlock1EndAddress;
			next->magicValue = UINT64_MAX-2;
		}
		else if(_usedBlock1EndAllocation == eIt-2)
		{
			// allocating the very last item before the ending one
			a = &(*_usedBlock1EndAllocation);
			_usedBlock1EndAllocation++;
		}
		else
		{
			// no more space in the AllocationBlock =>
			// allocate new one
			AllocationBlock& bPrev = _allocationBlockList1.back();
			AllocationBlock& bNew = createAllocationBlock();
			_allocationBlockList1.push_back(bNew);
			_usedBlock1EndAllocation = bNew.allocations.begin();
			_usedBlock1EndAllocation++;
			a = &(*_usedBlock1EndAllocation);
			_usedBlock1EndAllocation++;

			// link new and previous AllocationBlock
			LastAllocation& lPrev = reinterpret_cast<LastAllocation&>(bPrev.allocations.back());
			lPrev.address = addr;
			lPrev.nextAllocation = reinterpret_cast<FirstAllocation*>(&bNew.allocations.front());
			reinterpret_cast<FirstAllocation&>(bNew.allocations.front()).prevAllocation = &lPrev;

			// write special allocation after the current allocation to serve as stopper for some algorithms
			SpecialAllocation* next = reinterpret_cast<SpecialAllocation*>(&(*_usedBlock1EndAllocation));
			next->address = _usedBlock1EndAddress;
			next->magicValue = UINT64_MAX-2;
		}
	}

	// return the allocation
	return a;
}


// The resetAllocationBlock() method resets AllocationBlock2 and AllocationBlock1 pointers to
// empty AllocationBlock state. The function is expected to be called when empty state is detected.
// This might happen only when the allocation on the start address is freed.
template<typename AllocationRecord, size_t RecordsPerBlock>
void CircularAllocationMemory<AllocationRecord, RecordsPerBlock>::resetDataMemoryPointers(uint64_t aThisAddress,
	CircularAllocationMemory<AllocationRecord, RecordsPerBlock>* m) noexcept
{
	// does the last freed allocation belong to block2 or block1?
	if(aThisAddress == m->_usedBlock2StartAddress) {
		if(m->_usedBlock1EndAddress == m->_bufferStartAddress) {
			// empty Block1 -> reset Block2 pointers
			m->_usedBlock2EndAddress = m->_bufferStartAddress;
			m->_usedBlock2StartAddress = m->_bufferStartAddress;
			m->_usedBlock2EndAllocation = m->_allocationBlockList2.back().allocations.begin() + 1;
		}
		else {
			// Block1 not empty -> swap Block2 and Block1
			m->_usedBlock2EndAddress = m->_usedBlock1EndAddress;
			m->_usedBlock2StartAddress = m->_usedBlock1StartAddress;
			m->_usedBlock1EndAddress = m->_bufferStartAddress;
			m->_usedBlock1StartAddress = m->_bufferStartAddress;
			m->_allocationBlockList2.swap(m->_allocationBlockList1);
			m->_usedBlock2EndAllocation = m->_usedBlock1EndAllocation;
			m->_usedBlock1EndAllocation = m->_allocationBlockList1.back().allocations.begin() + 1;
		}
	}
	else
	// aThis->address == m->_usedBlock1StartAddress
	{
		// reset Block1 pointers
		m->_usedBlock1EndAddress = m->_bufferStartAddress;
		m->_usedBlock1StartAddress = m->_bufferStartAddress;
		m->_usedBlock1EndAllocation = m->_allocationBlockList1.back().allocations.begin() + 1;
	}
}


template<typename AllocationRecord, size_t RecordsPerBlock>
void CircularAllocationMemory<AllocationRecord, RecordsPerBlock>::destroyAllocation(AllocationRecord* a) noexcept
{
	SpecialAllocation* aThis = reinterpret_cast<SpecialAllocation*>(a);
	SpecialAllocation* aPrev = reinterpret_cast<SpecialAllocation*>(a - 1);
	assert(aThis->magicValue < UINT64_MAX-2 && "Allocation is already freed.");

	// mark this allocation as free by setting its magicValue member to UINT64_MAX;
	// the value UINT64_MAX-1 is used instead of UINT64_MAX if all previous allocations
	// from the beginning of the current AllocationBlock are free (e.g. they are marked with UINT64_MAX-1)
	// and for the first and the last internal allocation at the beginning and at the end of AllocationBlock,
	// the value UINT64_MAX-2 is used as stopper for some algorithms on the Allocation
	// that will be allocated as the next one
	if(aPrev->magicValue != UINT64_MAX-1)
		// mark just this one as free
		aThis->magicValue = UINT64_MAX;
	else
	{
		// mark this one as part of continous free block starting at the beginning of AllocationBlock
		aThis->magicValue = UINT64_MAX-1;
		aThis->index = aPrev->index + 1;
		aThis->circularAllocationMemory = aPrev->circularAllocationMemory;
		auto* m = aPrev->circularAllocationMemory;

		// iterate over following free allocations having magicValue set to UINT64_MAX,
		// iterating possibly until the end of AllocationBlock,
		// while setting magicValue to UINT64_MAX-1
		AllocationRecord* fwd = a + 1;
		SpecialAllocation* fwdThis = reinterpret_cast<SpecialAllocation*>(fwd);
		SpecialAllocation* fwdPrev = reinterpret_cast<SpecialAllocation*>(fwd - 1);
		while(fwdThis->magicValue == UINT64_MAX) {
			fwdThis->magicValue = UINT64_MAX-1;
			fwdThis->index = fwdPrev->index + 1;
			fwdThis->circularAllocationMemory = m;
			fwdPrev = fwdThis;
			fwd++;
			fwdThis = reinterpret_cast<SpecialAllocation*>(fwd);
		}

		// test if freeing the first allocation in the Block2 or Block1
		if(aThis->address == m->_usedBlock2StartAddress || aThis->address == m->_usedBlock1StartAddress)
		{
			// test if free block finished before the end of the first AllocationBlock
			// (on this point, magicValue can be any valid allocation (e.g. <UINT64_MAX-2)
			// or not valid allocation of magicValue UINT64_MAX-2, UINT64_MAX-1, but it cannot be UINT64_MAX)
			if(fwdThis->magicValue != UINT64_MAX-1)
			{
				if(fwdThis->magicValue == UINT64_MAX-2)
				{
					// magicValue is UINT64_MAX-2 => all allocations were destroyed => we just reset DataMemory pointers
					resetDataMemoryPointers(aThis->address, m);
					return;
				}
				else {
					// magicValue is valid allocation (<UINT64_MAX-2) => let's take its address
					if(aThis->address == m->_usedBlock2StartAddress)
						m->_usedBlock2StartAddress = fwdThis->address;
					else
						m->_usedBlock1StartAddress = fwdThis->address;
				}
			}
			else
			// free block extends to the end of the first AllocationBlock
			// (magicValue is UINT64_MAX-1 on this point)
			{
				AllocationRecord* p = reinterpret_cast<AllocationRecord*>(reinterpret_cast<LastAllocation*>(fwdThis)->nextAllocation);
				if(p == nullptr)
				{
					// no second AllocationBlock and nothing remains in the first one -> everything is freed
					resetDataMemoryPointers(aThis->address, m);
					return;
				}
				else
				// free block extends to the second AllocationBlock
				{
					// find the _usedBlock[1|2]StartAddress inside the second AllocationBlock
					p++;
					while(reinterpret_cast<SpecialAllocation*>(p)->magicValue == UINT64_MAX-1)
						if(reinterpret_cast<SpecialAllocation*>(p)->index != RecordsPerBlock+2-1)
							p++;
						else {
							resetDataMemoryPointers(aThis->address, m);
							goto afterSettingAddress;
						}

					// did we reached the end of allocations?
					if(reinterpret_cast<SpecialAllocation*>(p)->magicValue == UINT64_MAX-2) {
						// no more allocations in the Block[1|2]
						resetDataMemoryPointers(aThis->address, m);
					}
					else {
						// we reached the oldest allocation in the Block[1|2]
						if(aThis->address == m->_usedBlock2StartAddress)
							m->_usedBlock2StartAddress = reinterpret_cast<SpecialAllocation*>(p)->address;
						else // aThis->address == m->_usedBlock1StartAddress
							m->_usedBlock1StartAddress = reinterpret_cast<SpecialAllocation*>(p)->address;
					}
					afterSettingAddress:;
				}
			}
		}

		// if all allocations are free in AllocationBlock, destroy AllocationBlock
		if(fwdThis->magicValue == UINT64_MAX-1 && fwdThis->index == RecordsPerBlock+2-1) {
			AllocationBlock* b = reinterpret_cast<AllocationBlock*>(reinterpret_cast<uint8_t*>(fwdThis) -
				size_t(&static_cast<AllocationBlock*>(nullptr)->allocations[RecordsPerBlock+2-1]));
			if(b == &m->_allocationBlockList2.back()) {
				m->destroyAllocationBlock(*b);
				if(!m->_allocationBlockList2.empty())
					m->_usedBlock2EndAllocation = m->_allocationBlockList2.back().allocations.end() - 1;
			}
			else if(b == &m->_allocationBlockList1.back()) {
				m->destroyAllocationBlock(*b);
				if(!m->_allocationBlockList1.empty())
					m->_usedBlock1EndAllocation = m->_allocationBlockList1.back().allocations.end() - 1;
			}
		}
	}
}


template<typename AllocationRecord, size_t RecordsPerBlock>
typename CircularAllocationMemory<AllocationRecord, RecordsPerBlock>::AllocationBlock&
	CircularAllocationMemory<AllocationRecord, RecordsPerBlock>::createAllocationBlock()
{
	if(_allocationBlockRecycleList.empty()) {
		AllocationBlock& b = *new AllocationBlock;
		FirstAllocation& f = reinterpret_cast<FirstAllocation&>(b.allocations.front());
		f.prevAllocation = 0;
		f.magicValue = UINT64_MAX-1;
		f.index = 0;
		f.circularAllocationMemory = this;
		LastAllocation& l = reinterpret_cast<LastAllocation&>(b.allocations.back());
		l.address = 0;
		l.magicValue = UINT64_MAX-1;
		l.index = RecordsPerBlock+2-1;
		l.nextAllocation = 0;
		return b;
	}
	else {
		AllocationBlock& b = _allocationBlockRecycleList.back();
		_allocationBlockRecycleList.pop_back();
		FirstAllocation& f = reinterpret_cast<FirstAllocation&>(b.allocations.front());
		f.prevAllocation = 0;
		LastAllocation& l = reinterpret_cast<LastAllocation&>(b.allocations.back());
		l.address = 0;
		l.nextAllocation = 0;
		return b;
	}
}


template<typename AllocationRecord, size_t RecordsPerBlock>
void CircularAllocationMemory<AllocationRecord, RecordsPerBlock>::destroyAllocationBlock(AllocationBlock& b) noexcept
{
	// delete this AllocationBlock from the list (linked through First and Last Allocation)
	FirstAllocation& f = reinterpret_cast<FirstAllocation&>(b.allocations.front());
	LastAllocation& l = reinterpret_cast<LastAllocation&>(b.allocations.back());
	if(f.prevAllocation)
		f.prevAllocation->nextAllocation = l.nextAllocation;
	if(l.nextAllocation)
		l.nextAllocation->prevAllocation = f.prevAllocation;

	// recycle or delete AllocationBlock
	if(_allocationBlockRecycleList.empty() ||
	   &_allocationBlockRecycleList.front() == &_allocationBlockRecycleList.back())
	{
		b.unlink();
		_allocationBlockRecycleList.push_back(b);
	}
	else
		delete &b;
}


template<typename AllocationRecord, size_t RecordsPerBlock>
template<typename... Args>
AllocationRecord* CircularAllocationMemory<AllocationRecord, RecordsPerBlock>::allocInternal(size_t numBytes, Args... args)
{
	// zero size allocations are forbidden
	// (if this should be changed, verify all related code before enabling them)
	assert(numBytes!=0 && "CircularAllocationMemory::allocInternal() called with numBytes equal to zero.");

	// try to alloc at the end of usedBlock2
	if(_usedBlock2EndAddress + numBytes <= _bufferEndAddress)
	{
		// align the allocation to 64 or to 16 bytes
		uint64_t addr =
			(numBytes >= 64)
				? ((_usedBlock2EndAddress+0x3f) & ~0x3f)   // align to 64 bytes
				: ((_usedBlock2EndAddress+0x0f) & ~0x0f);  // align to 16 bytes

		if(addr + numBytes <= _bufferEndAddress)
		{
			// update variables
			uint64_t newUsedBlock2EndAddress = addr + numBytes;
			_usedBytes += newUsedBlock2EndAddress - _usedBlock2EndAddress;
			_usedBlock2EndAddress = newUsedBlock2EndAddress;

			// create DataAllocation
			AllocationRecord* a = createAllocation2(addr);
			a->init(addr, numBytes, args...);
			return a;
		}
	}

	// try to alloc at the end of usedBlock1 that is placed on the beginning of the buffer
	// (usedBlock1 always starts on the beginning of the buffer;
	// a kind of cleaning+compacting process takes records from the beginning of the buffer, discards freed allocations,
	// moves still valid allocations to new place, compacting them, and creating new continous free space;
	// this free space is then reused for new allocations as usedBlock1;
	// when usedBlock2 becomes empty as the result of cleaning+compacting process, usedBlock1 is turned to usedBlock2
	// and usedBlock2 becomes empty block at the beginning of the buffer)
	if(_usedBlock1EndAddress + numBytes <= _usedBlock2StartAddress)
	{
		// align the allocation to 64 or to 16 bytes
		uint64_t addr =
			(numBytes >= 64)
				? ((_usedBlock1EndAddress+0x3f) & ~0x3f)
				: ((_usedBlock1EndAddress+0x0f) & ~0x0f);

		if(addr + numBytes <= _usedBlock2StartAddress)
		{
			// update variables
			uint64_t newUsedBlock1EndAddress = addr + numBytes;
			_usedBytes += newUsedBlock1EndAddress - _usedBlock1EndAddress;
			_usedBlock1EndAddress = newUsedBlock1EndAddress;

			// create DataAllocation
			AllocationRecord* a = createAllocation1(addr);
			a->init(addr, numBytes, args...);
			return a;
		}
	}

	// not enough continuous space in this DataMemory
	return nullptr;
}


template<typename AllocationRecord, size_t RecordsPerBlock>
void CircularAllocationMemory<AllocationRecord, RecordsPerBlock>::freeInternal(AllocationRecord* a) noexcept
{
	// get nextAddress
	// (the address of the next allocation)
	uint64_t nextAddress;
	AllocationRecord* n = a + 1;
	auto magicValue = reinterpret_cast<SpecialAllocation*>(n)->magicValue;
	if(magicValue != UINT64_MAX-1) {
		// set nextAddress from next allocation's address field
		//
		// Following cases are handled here:
		// - if magicValue <  UINT64_MAX-2 => it is valid allocation, so we return its address
		// - if magicValue == UINT64_MAX-2 => it is next-to-be-allocated allocation and we return the address it contains
		// - if magicValue == UINT64_MAX   => it is freed allocation and we return its original address
		nextAddress = reinterpret_cast<SpecialAllocation*>(n)->address;
	}
	else {
		// handle the end of the AllocationBlock
		//
		// if magicValue == UINT64_MAX-1 => it indicates the last element in AllocationBlock
		if(reinterpret_cast<LastAllocation*>(n)->address == 0)
			// no newer allocation => return _usedBlock[1|2]EndAddress
			nextAddress =
				(_usedBlock1EndAddress < reinterpret_cast<SpecialAllocation*>(a)->address)
				? _usedBlock2EndAddress
				: _usedBlock1EndAddress;
		else {
			// next AllocationBlock was destroyed => return remembered address of next allocation
			nextAddress = reinterpret_cast<LastAllocation*>(n)->address;
		}
	}

	// update _usedBytes
	_usedBytes -= nextAddress - reinterpret_cast<SpecialAllocation*>(a)->address;

	// destroy allocation
	destroyAllocation(a);
}


}
