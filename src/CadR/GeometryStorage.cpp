#include <CadR/GeometryStorage.h>
#include <CadR/GeometryMemory.h>
#include <CadR/PrimitiveSet.h>
#include <CadR/Renderer.h>
#include <CadR/StagingBuffer.h>
#include <CadR/VulkanDevice.h>
#include <iostream> // for cerr
#include <memory>
#include <numeric>

using namespace std;
using namespace CadR;



GeometryStorage::GeometryStorage(const AttribSizeList& attribSizeList)
	: GeometryStorage(Renderer::get(), attribSizeList)
{
}


GeometryStorage::GeometryStorage(Renderer* renderer, const AttribSizeList& attribSizeList)
	: _renderer(renderer)
	, _attribSizeList(attribSizeList)
{
	// compute amount of memory that each vertex consumes
	uint64_t vertexAttribSize = accumulate(attribSizeList.begin(), attribSizeList.end(), uint64_t(0));
	uint64_t vertexIndexSize = sizeof(uint32_t) * 6;
	uint64_t vertexPrimitiveSetSize = sizeof(PrimitiveSetGpuData) / 8;
	static_assert(sizeof(PrimitiveSetGpuData) % 8 == 0, "Size of PrimitiveSetGpuData must be divisible by 8, or redesign algorithm bellow.");
	uint64_t vertexSize = vertexAttribSize + vertexIndexSize + vertexPrimitiveSetSize;

	// compute maximum number of vertices in the largest buffer (2^64 bytes) 
	uint64_t maxVertices = UINT64_MAX / vertexSize;
	if(UINT64_MAX % vertexSize == vertexSize-1)  // if there is vertexSize-1 reminder, one more vertex will fit into 0x10000000000000000 (UINT64_MAX+1) sized buffer
		maxVertices++;

	// compute fractions of memory consumed by vertices, by indices and by primitiveSets
	// (this is stored as Unorm, e.g. although number is 0..2^64-1, it represents the value in 0..1 range)
	_vertexMemoryFractionUnorm = vertexAttribSize * maxVertices;
	_indexMemoryFractionUnorm = vertexIndexSize * maxVertices;
	_primitiveSetMemoryFractionUnorm = vertexPrimitiveSetSize * maxVertices;
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
