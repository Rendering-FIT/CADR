#include <CadR/HandleTable.h>
#include <CadR/DataStorage.h>  // some DataAllocation and HandlelessAllocation functions are placed there as a part of circular include problem
#include <CadR/Exceptions.h>

using namespace std;
using namespace CadR;



void HandleTable::LastLevelTable::finalize(HandleTable& handleTable) noexcept
{
	handleTable.destroy(handle);
	handle = 0;
	allocation.free();
}


void HandleTable::RoutingTable::finalize(HandleTable& handleTable) noexcept
{
	handleTable.destroy(handle);
	handle = 0;
	allocation.free();
}


HandleTable::LastLevelTable::LastLevelTable(DataStorage& storage) noexcept
	: allocation(storage)
	, handle(0)
	, addrList{}
{
}


HandleTable::RoutingTable::RoutingTable(DataStorage& storage) noexcept
	: allocation(storage)
	, handle(0)
	, addrList{}
{
}


void HandleTable::LastLevelTable::init(HandleTable& handleTable)
{
	// fill staging data with zeros
	size_t size = numHandlesPerTable * sizeof(uint64_t);
	StagingData stagingData = allocation.alloc(size);
	memset(stagingData.data(), 0, size);

	// alloc handle
	handleTable._highestHandle++;
	handle = handleTable._highestHandle;
	handleTable.set(handle, allocation.deviceAddress());
}


void HandleTable::RoutingTable::init(HandleTable& handleTable)
{
	// fill staging data with zeros
	size_t size = numHandlesPerTable * sizeof(uint64_t);
	StagingData stagingData = allocation.alloc(size);
	memset(stagingData.data(), 0, size);

	// alloc handle
	handleTable._highestHandle++;
	handle = handleTable._highestHandle;
	handleTable.set(handle, allocation.deviceAddress());
}


void HandleTable::LastLevelTable::setValue(unsigned index, uint64_t value)
{
	addrList[index] = value;
	StagingData stagingData = allocation.createStagingData();
	uint64_t* a = stagingData.data<uint64_t>();
	if(stagingData.needInit())
		memcpy(a, addrList.data(), addrList.size()*sizeof(uint64_t));
	else
		a[index] = value;
}


void HandleTable::RoutingTable::setValue(unsigned index, uint64_t value)
{
	addrList[index] = value;
	StagingData stagingData = allocation.createStagingData();
	uint64_t* a = stagingData.data<uint64_t>();
	if(stagingData.needInit())
		memcpy(a, addrList.data(), addrList.size()*sizeof(uint64_t));
	else
		a[index] = value;
}



HandleTable::HandleTable(DataStorage& storage)
	: _storage(&storage)
{
}


HandleTable::~HandleTable() noexcept
{
	destroyAll();
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


uint64_t HandleTable::createHandle0()
{
	LastLevelTable* llt;
	try {
		llt = new LastLevelTable(*_storage);
		llt->init(*this);
	} catch(...) {
		llt->finalize(*this);
		delete llt;
		throw;
	}

	llt->setValue(1, llt->allocation.deviceAddress());
	_level0 = llt;
	_handleLevel = 1;
	_createHandle = &HandleTable::createHandle1;
	_setHandle = &HandleTable::setHandle1;
	_rootTableDeviceAddress = &HandleTable::rootTableDeviceAddress1;

	_highestHandle = 2;
	return _highestHandle;
}


uint64_t HandleTable::createHandle1()
{
	// return handle from LastLevelTable unless it is almost full
	// (we reserve few last handles to allocate one more LastLevelTable and extra RoutingTables) 
	if(_highestHandle != (handleBitsLevelMask - 2)) {
		_highestHandle++;
		return _highestHandle;
	}

	// upgrade from one table level to two table levels
	RoutingTable* rt;
	LastLevelTable* llt;
	try {
		rt = new RoutingTable(*_storage);
		llt = new LastLevelTable(*_storage);
		rt->init(*this);
		llt->init(*this);
	} catch(...) {
		rt->finalize(*this);
		llt->finalize(*this);
		delete rt;
		delete llt;
		throw;
	}
	rt->childTableList[0].lastLevelTable = _level0;
	rt->childTableList[1].lastLevelTable = llt;
	rt->setValue(0, _level0->allocation.deviceAddress());
	rt->setValue(1, llt->allocation.deviceAddress());
	_level1 = rt;
	_handleLevel = 2;
	_createHandle = &HandleTable::createHandle2;
	_setHandle = &HandleTable::setHandle2;
	_rootTableDeviceAddress = &HandleTable::rootTableDeviceAddress2;

	_highestHandle++;
	return _highestHandle;
}


uint64_t HandleTable::createHandle2()
{
	// return handle from LastLevelTable unless it is almost full
	// (we reserve few last handles to allocate one more LastLevelTable and extra RoutingTables) 
	if((_highestHandle & handleBitsLevelMask) != (handleBitsLevelMask - 3)) {
		_highestHandle++;
		return _highestHandle;
	}

	// append one LastLevelTable
	LastLevelTable* llt;
	try {
		llt = new LastLevelTable(*_storage);
		llt->init(*this);
	} catch(...) {
		llt->finalize(*this);
		delete llt;
		throw;
	}
	unsigned index = unsigned(_highestHandle >> handleBitsLevelShift) + 1;
	_level1->childTableList[index].lastLevelTable = llt;
	_level1->setValue(index, llt->allocation.deviceAddress());

	_highestHandle++;
	return _highestHandle;
}
