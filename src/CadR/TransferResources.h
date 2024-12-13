#ifndef CADR_TRANSFER_RESOURCES_HEADER
# define CADR_TRANSFER_RESOURCES_HEADER

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

#endif


// inline functions
#if !defined(CADR_TRANSFER_RESOURCES_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_TRANSFER_RESOURCES_INLINE_FUNCTIONS
namespace CadR {

inline TransferResources::TransferResources(size_t capacity)  { _resourceList.reserve(capacity); }
inline void TransferResources::release()  { for(auto t : _resourceList) std::get<0>(t)->uploadDone(std::get<1>(t), std::get<2>(t)); _resourceList.clear(); }
inline TransferResources::~TransferResources()  { for(auto t : _resourceList) std::get<0>(t)->uploadDone(std::get<1>(t), std::get<2>(t)); }
inline void TransferResources::append(DataMemory* m, void* id1, void* id2)  { _resourceList.emplace_back(m, id1, id2); }
inline TransferResources::TransferResources(TransferResources&& other) : _resourceList(std::move(other._resourceList))  {}
inline TransferResources& TransferResources::operator=(TransferResources&& other)  { _resourceList = std::move(other._resourceList); return *this; }

inline TransferResourcesReleaser::TransferResourcesReleaser() : _dataStorage(nullptr)  {}
inline TransferResourcesReleaser::TransferResourcesReleaser(Id id, DataStorage* dataStorage) : _id(id), _dataStorage(dataStorage)  {}
inline void TransferResourcesReleaser::release()  { if(_dataStorage==nullptr) return; _dataStorage->uploadDone(_id); _dataStorage=nullptr; }
inline TransferResourcesReleaser::~TransferResourcesReleaser()  { if(_dataStorage!=nullptr) _dataStorage->uploadDone(_id); }
inline TransferResourcesReleaser::TransferResourcesReleaser(TransferResourcesReleaser&& other) : _id(other._id), _dataStorage(other._dataStorage)  { other._dataStorage=nullptr; }
inline TransferResourcesReleaser& TransferResourcesReleaser::operator=(TransferResourcesReleaser&& other)  { if(_dataStorage!=nullptr) _dataStorage->uploadDone(_id); _id=other._id; _dataStorage=other._dataStorage; other._dataStorage=nullptr; return *this; }

}
#endif
