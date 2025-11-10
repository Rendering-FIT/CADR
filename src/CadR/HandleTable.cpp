#include <CadR/HandleTable.h>
#include <CadR/DataAllocation.h>
#include <CadR/StagingData.h>

using namespace std;
using namespace CadR;



void HandleTable::LastLevelTable::finalize(HandleTable& handleTable) noexcept
{
	allocation.free();
}


void HandleTable::RoutingTable::finalize(HandleTable& handleTable) noexcept
{
	allocation.free();
}


HandleTable::LastLevelTable::LastLevelTable(DataStorage& storage) noexcept
	: allocation(storage)
	, addrList{}
{
}


HandleTable::RoutingTable::RoutingTable(DataStorage& storage) noexcept
	: allocation(storage)
	, addrList{}
{
}


void HandleTable::LastLevelTable::init(HandleTable& handleTable)
{
	// fill staging data with zeros
	constexpr size_t size = numHandlesPerTable * sizeof(uint64_t);
	StagingData stagingData = allocation.alloc(size);
	memset(stagingData.data(), 0, size);
}


void HandleTable::RoutingTable::init(HandleTable& handleTable)
{
	// fill staging data with zeros
	constexpr size_t size = numHandlesPerTable * sizeof(uint64_t);
	StagingData stagingData = allocation.alloc(size);
	memset(stagingData.data(), 0, size);
}


bool HandleTable::LastLevelTable::setValue(unsigned index, uint64_t value)
{
	addrList[index] = value;
	StagingData stagingData = allocation.createStagingData();
	uint64_t* a = stagingData.data<uint64_t>();
	if(stagingData.wasReallocated())
		memcpy(a, addrList.data(), addrList.size()*sizeof(uint64_t));
	else
		a[index] = value;
	return stagingData.wasReallocated();
}


bool HandleTable::RoutingTable::setValue(unsigned index, uint64_t value)
{
	addrList[index] = value;
	StagingData stagingData = allocation.createStagingData();
	uint64_t* a = stagingData.data<uint64_t>();
	if(stagingData.wasReallocated())
		memcpy(a, addrList.data(), addrList.size()*sizeof(uint64_t));
	else
		a[index] = value;
	return stagingData.wasReallocated();
}



void HandleTable::destroyAll() noexcept
{
	if(_createHandle == &HandleTable::createHandle0) {
	}
	else if(_createHandle == &HandleTable::createHandle1) {
		_level0->finalize(*this);
		delete _level0;
		_createHandle = &HandleTable::createHandle0;
	}
	else if(_createHandle == &HandleTable::createHandle2) {
		uint64_t maxIndex = (_highestHandle >> handleBitsLevelShift);
		for(uint64_t i=maxIndex; i>=1; i--) {
			LastLevelTable* llt = _level1->childTableList[i].lastLevelTable;
			llt->finalize(*this);
			delete llt;
		}
		LastLevelTable* llt = _level1->childTableList[0].lastLevelTable;
		_level1->finalize(*this);
		delete _level1;
		_setHandle = &HandleTable::setHandle1;
		llt->finalize(*this);
		delete llt;
		_createHandle = &HandleTable::createHandle0;
	}
}


// createHandle0() is called when _highestHandle is 0
uint64_t HandleTable::createHandle0()
{
	// create the first LastLevelTable
	LastLevelTable* llt = nullptr;
	try {
		llt = new LastLevelTable(*_storage);
		llt->init(*this);
	} catch(...) {
		if(llt) {
			llt->finalize(*this);
			delete llt;
		}
		throw;
	}

	// update HandleTable
	_level0 = llt;
	_handleLevel = 1;
	_createHandle = &HandleTable::createHandle1;
	_setHandle = &HandleTable::setHandle1;
	_rootTableDeviceAddress = &HandleTable::rootTableDeviceAddress1;

	// return new handle
	_highestHandle = 1;
	return _highestHandle;
}


// createHandle1() is called when _highestHandle is 1..handleBitsLevelMask
uint64_t HandleTable::createHandle1()
{
	// return handle from LastLevelTable unless it is full
	if(_highestHandle != handleBitsLevelMask) {
		_highestHandle++;
		return _highestHandle;
	}

	// upgrade from one table level to two table levels
	RoutingTable* l1 = nullptr;
	LastLevelTable* llt = nullptr;
	try {
		l1 = new RoutingTable(*_storage);
		llt = new LastLevelTable(*_storage);
		l1->init(*this);
		llt->init(*this);
	} catch(...) {
		if(l1) {
			l1->finalize(*this);
			delete l1;
		}
		if(llt) {
			llt->finalize(*this);
			delete llt;
		}
		throw;
	}

	// initialize l1
	// (we have two LastLevelTables and one RoutingTable (L1))
	l1->childTableList[0].lastLevelTable = _level0;
	l1->childTableList[1].lastLevelTable = llt;
	l1->setValue(0, _level0->allocation.deviceAddress());
	l1->setValue(1, llt->allocation.deviceAddress());

	// update HandleTable
	_level1 = l1;
	_handleLevel = 2;
	_createHandle = &HandleTable::createHandle2;
	_setHandle = &HandleTable::setHandle2;
	_rootTableDeviceAddress = &HandleTable::rootTableDeviceAddress2;

	// return new handle
	_highestHandle++;
	return _highestHandle;
}


// createHandle2() is called when _highestHandle is numHandlesPerTable..sqr(numHandlesPerTable)-1
uint64_t HandleTable::createHandle2()
{
	// return handle from LastLevelTable unless it is full
	if((_highestHandle & handleBitsLevelMask) != handleBitsLevelMask) {
		_highestHandle++;
		return _highestHandle;
	}

	// append one more LastLevelTable
	LastLevelTable* llt = nullptr;
	try {
		llt = new LastLevelTable(*_storage);
		llt->init(*this);
	} catch(...) {
		if(llt) {
			llt->finalize(*this);
			delete llt;
		}
		throw;
	}

	unsigned l1Index = unsigned(_highestHandle >> handleBitsLevelShift) + 1;
	if(l1Index < numHandlesPerTable) {

		// update RoutingTable _level1
		_level1->childTableList[l1Index].lastLevelTable = llt;
		_level1->setValue(l1Index, llt->allocation.deviceAddress());

		// return new handle
		_highestHandle++;
		return _highestHandle;

	}
	else {

		// upgrade from two table levels to three table levels
		RoutingTable* l1 = nullptr;
		RoutingTable* l2 = nullptr;
		try {
			l1 = new RoutingTable(*_storage);
			l2 = new RoutingTable(*_storage);
			l1->init(*this);
			l2->init(*this);
		} catch(...) {
			if(l1) {
				l1->finalize(*this);
				delete l1;
			}
			if(l2) {
				l2->finalize(*this);
				delete l2;
			}
			throw;
		}

		// initialize l1
		l1->childTableList[0].lastLevelTable = llt;
		l1->setValue(0, llt->allocation.deviceAddress());

		// initialize l2
		l2->childTableList[0].routingTable = _level1;
		l2->childTableList[1].routingTable = l1;
		l2->setValue(0, _level1->allocation.deviceAddress());
		l2->setValue(1, l1->allocation.deviceAddress());

		// update HandleTable
		_level2 = l2;
		_handleLevel = 3;
		_createHandle = &HandleTable::createHandle3;
		_setHandle = &HandleTable::setHandle3;
		_rootTableDeviceAddress = &HandleTable::rootTableDeviceAddress3;

		// return new handle
		_highestHandle++;
		return _highestHandle;

	}
}


// createHandle3() is called when _highestHandle is sqr(numHandlesPerTable)..
uint64_t HandleTable::createHandle3()
{
	// return handle from LastLevelTable unless it is full
	if((_highestHandle & handleBitsLevelMask) != handleBitsLevelMask) {
		_highestHandle++;
		return _highestHandle;
	}

	// append one more LastLevelTable
	LastLevelTable* llt = nullptr;
	try {
		llt = new LastLevelTable(*_storage);
		llt->init(*this);
	} catch(...) {
		if(llt) {
			llt->finalize(*this);
			delete llt;
		}
		throw;
	}

	unsigned l1Index = ((_highestHandle >> handleBitsLevelShift) & handleBitsLevelMask) + 1;
	if(l1Index < numHandlesPerTable) {

		// update RoutingTable l1 and l2 if needed
		unsigned l2Index = unsigned(_highestHandle >> (2*handleBitsLevelShift));
		RoutingTable* l1 = _level2->childTableList[l2Index].routingTable;
		l1->childTableList[l1Index].lastLevelTable = llt;
		bool l1Reallocated = l1->setValue(l1Index, llt->allocation.deviceAddress());
		if(l1Reallocated)
			_level2->setValue(l2Index, l1->allocation.deviceAddress());

		// return new handle
		_highestHandle++;
		return _highestHandle;

	}
	else {

		// append one more level1 table
		RoutingTable* l1 = nullptr;
		try {
			l1 = new RoutingTable(*_storage);
			l1->init(*this);
		} catch(...) {
			if(l1) {
				l1->finalize(*this);
				delete l1;
			}
			throw;
		}

		// initialize l1
		l1->childTableList[0].lastLevelTable = llt;
		l1->setValue(0, llt->allocation.deviceAddress());

		// update l2
		unsigned l2Index = unsigned(_highestHandle >> (2*handleBitsLevelShift)) + 1;
		_level2->childTableList[l2Index].routingTable = l1;
		_level2->setValue(l2Index, l1->allocation.deviceAddress());

		// return new handle
		_highestHandle++;
		return _highestHandle;

	}
}


// function is not inline because it is called using function pointer
void HandleTable::setHandle0(uint64_t, uint64_t)
{
}


// function is not inline because it is called using function pointer
void HandleTable::setHandle1(uint64_t handle, uint64_t addr)
{
	_level0->setValue(unsigned(handle), addr);
}


// function is not inline because it is called using function pointer
void HandleTable::setHandle2(uint64_t handle, uint64_t addr)
{
	unsigned l1Index = unsigned(handle >> handleBitsLevelShift);
	LastLevelTable* llt = _level1->childTableList[l1Index].lastLevelTable;
	bool lltReallocated = llt->setValue(unsigned(handle & handleBitsLevelMask), addr);
	if(lltReallocated)
		_level1->setValue(l1Index, llt->allocation.deviceAddress());
}


// function is not inline because it is called using function pointer
void HandleTable::setHandle3(uint64_t handle, uint64_t addr)
{
	unsigned l1Index = (handle >> handleBitsLevelShift) & handleBitsLevelMask;
	unsigned l2Index = unsigned(handle >> (2*handleBitsLevelShift));
	RoutingTable* l1 = _level2->childTableList[l2Index].routingTable;
	LastLevelTable* llt = l1->childTableList[l1Index].lastLevelTable;
	bool lltReallocated = llt->setValue(unsigned(handle & handleBitsLevelMask), addr);
	if(lltReallocated) {
		bool l1Reallocated = l1->setValue(l1Index, llt->allocation.deviceAddress());
		if(l1Reallocated)
			_level2->setValue(l2Index, l1->allocation.deviceAddress());
	}
}


// function is not inline because it is called using function pointer
uint64_t HandleTable::rootTableDeviceAddress0() const
{
	return 0;
}


// function is not inline because it is called using function pointer
uint64_t HandleTable::rootTableDeviceAddress1() const
{
	return _level0->allocation.deviceAddress();
}


// function is not inline because it is called using function pointer
uint64_t HandleTable::rootTableDeviceAddress2() const
{
	return _level1->allocation.deviceAddress();
}


// function is not inline because it is called using function pointer
uint64_t HandleTable::rootTableDeviceAddress3() const
{
	return _level2->allocation.deviceAddress();
}
