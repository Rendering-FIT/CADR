#include <CadR/GeometryStorage.h>
#include <CadR/GeometryMemory.h>
#include <CadR/Renderer.h>
#include <CadR/StagingBuffer.h>
#include <CadR/VulkanDevice.h>
#include <iostream> // for cerr
#include <memory>
#include <numeric>

using namespace std;
using namespace CadR;



GeometryStorage::GeometryStorage(const AttribSizeList& attribSizeList, size_t initialVertexCapacity)
	: GeometryStorage(Renderer::get(), attribSizeList, initialVertexCapacity, initialVertexCapacity*6, initialVertexCapacity/8)
{
}


GeometryStorage::GeometryStorage(Renderer* r, const AttribSizeList& attribSizeList, size_t initialVertexCapacity)
	: GeometryStorage(r, attribSizeList, initialVertexCapacity, initialVertexCapacity*6, initialVertexCapacity/8)
{
}


GeometryStorage::GeometryStorage(const AttribSizeList& attribSizeList, size_t initialVertexCapacity,
	                             size_t initialIndexCapacity, size_t initialPrimitiveSetCapacity)
	: GeometryStorage(Renderer::get(), attribSizeList, initialVertexCapacity, initialIndexCapacity, initialPrimitiveSetCapacity)
{
}


GeometryStorage::GeometryStorage(Renderer* renderer, const AttribSizeList& attribSizeList,
	                             size_t initialVertexCapacity, size_t initialIndexCapacity, size_t initialPrimitiveSetCapacity)
	: _renderer(renderer)
	, _attribSizeList(attribSizeList)
{
	if(initialVertexCapacity!=0 || initialIndexCapacity!=0 || initialPrimitiveSetCapacity!=0)
		_geometryMemoryList.emplace_back(make_unique<GeometryMemory>(
			this, initialVertexCapacity, initialIndexCapacity, initialPrimitiveSetCapacity));
}


GeometryStorage::~GeometryStorage()
{
	// destroy VertexMemory objects
	_geometryMemoryList.clear();
}


void GeometryStorage::cancelAllAllocations()
{
	for(unique_ptr<GeometryMemory>& m : _geometryMemoryList)
		m->cancelAllAllocations();
}


void GeometryStorage::freeGeometryMemoryId(uint32_t id) noexcept
{
	if(id==_highestGeometryMemoryId) {
		_highestGeometryMemoryId--;
		for(size_t i=0; i<_freeGeometryMemoryIds.size(); i++)
			if(_freeGeometryMemoryIds[i] == _highestGeometryMemoryId) {
				_highestGeometryMemoryId--;
				continue;
			}
			else {
				_freeGeometryMemoryIds.erase(_freeGeometryMemoryIds.begin(), _freeGeometryMemoryIds.begin()+i);
				return;
			}
		_freeGeometryMemoryIds.clear();
	}
	else {
		// insert id into the _freeGeometryMemoryIds
		if(_freeGeometryMemoryIds.empty())
			_freeGeometryMemoryIds.push_back(id);
		else {
			// make the vector sorted from highest to lowest
			size_t i = _freeGeometryMemoryIds.size();
			while(true) {
				i--;
				if(_freeGeometryMemoryIds[i] > id || i == 0) {
					_freeGeometryMemoryIds.insert(_freeGeometryMemoryIds.begin()+i, id);
					return;
				}
			}
		}
	}
}
