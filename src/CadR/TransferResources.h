#pragma once

#include <list>
#include <tuple>
#include <vector>

namespace CadR {

class DataStorage;


class TransferResources {
protected:
	std::vector<std::tuple<DataMemory*, void*, void*>> _resourceList;
public:
	TransferResources() = default;
	TransferResources(size_t capacity);
	void release();
	~TransferResources();

	void append(DataMemory* m, void*, void*);

	TransferResources(TransferResources&& other);
	TransferResources(const TransferResources&) = delete;
	TransferResources& operator=(TransferResources&& other);
	TransferResources& operator=(const TransferResources&) = delete;
};


class TransferResourcesReleaser {
public:
	//using InProgressList = std::list<std::vector<std::tuple<StagingMemory*,uint64_t>>>;
	using Id = std::list<TransferResources>::iterator;
protected:
	Id _id;
	DataStorage* _dataStorage;
public:
	TransferResourcesReleaser();
	TransferResourcesReleaser(Id id, DataStorage* dataStorage);
	void release();
	~TransferResourcesReleaser();

	TransferResourcesReleaser(const TransferResourcesReleaser&) = delete;
	TransferResourcesReleaser(TransferResourcesReleaser&& other);
	TransferResourcesReleaser& operator=(const TransferResourcesReleaser&) = delete;
	TransferResourcesReleaser& operator=(TransferResourcesReleaser&& other);
};

}

// inline functions
namespace CadR {
inline TransferResources::TransferResources(size_t capacity)  { _resourceList.reserve(capacity); }
inline void TransferResources::release()  { for(auto t : _resourceList) std::get<0>(t)->uploadDone(std::get<1>(t), std::get<2>(t)); _resourceList.clear(); }
inline TransferResources::~TransferResources()  { for(auto t : _resourceList) std::get<0>(t)->uploadDone(std::get<1>(t), std::get<2>(t)); }
inline void TransferResources::append(DataMemory* m, void* id1, void* id2)  { _resourceList.emplace_back(m, id1, id2); }
inline TransferResources::TransferResources(TransferResources&& other) : _resourceList(std::move(other._resourceList))  {}
inline TransferResources& TransferResources::operator=(TransferResources&& other)  { _resourceList = std::move(other._resourceList); return *this; }

inline TransferResourcesReleaser::TransferResourcesReleaser() : _dataStorage(nullptr)  {}
inline TransferResourcesReleaser::TransferResourcesReleaser(Id id, DataStorage* dataStorage) : _id(id), _dataStorage(dataStorage)  {}
inline TransferResourcesReleaser::TransferResourcesReleaser(TransferResourcesReleaser&& other) : _id(other._id), _dataStorage(other._dataStorage)  { other._dataStorage=nullptr; }
}

// include inline Transfer functions defined in DataStorage.h (they are defined there because they depend on CadR::DataStorage class)
#include <CadR/DataStorage.h>
