#include <CadR/Geometry.h>
#include <CadR/PrimitiveSet.h>

using namespace std;
using namespace CadR;


Geometry::Geometry(Geometry&& g)
	: _geometryMemory(g._geometryMemory)
	, _vertexDataID(g._vertexDataID)
	, _indexDataID(g._indexDataID)
	, _primitiveSetDataID(g._primitiveSetDataID)
	, _drawableList(std::move(g._drawableList))
{
	Renderer* r = renderer();
	g._geometryMemory = r->emptyGeometryMemory();
	g._vertexDataID = 0;
	g._indexDataID = 0;
	g._primitiveSetDataID = 0;
	vertexAllocation().owner = this;
	indexAllocation().owner = this;
	primitiveSetAllocation().owner = this;
}


Geometry& Geometry::operator=(Geometry&& g)
{
	Renderer* r = renderer();
	_geometryMemory = g._geometryMemory;
	g._geometryMemory = r->emptyGeometryMemory();
	_vertexDataID = g._vertexDataID;
	_indexDataID = g._indexDataID;
	_primitiveSetDataID = g._primitiveSetDataID;
	_drawableList = std::move(g._drawableList);
	g._vertexDataID = 0;
	g._indexDataID = 0;
	g._primitiveSetDataID = 0;
	vertexAllocation().owner = this;
	indexAllocation().owner = this;
	primitiveSetAllocation().owner = this;
	return *this;
}


void Geometry::alloc(GeometryStorage* geometryStorage, size_t numVertices, size_t numIndices, size_t numPrimitiveSets)
{
	// free previous data (if any)
	free();

	// try alloc inside existing GeometryMemory objects
	auto& l = geometryStorage->geometryMemoryList();
	for(unique_ptr<GeometryMemory>& m : l) {
		if(m->tryAlloc(numVertices, numIndices, numPrimitiveSets,
		              _vertexDataID, _indexDataID, _primitiveSetDataID))
		{
			_geometryMemory = m.get();
			return;
		}
	}

	// collect info for new GeometryMemory object
	size_t totalVerticesAllocated = 0;
	size_t totalIndicesAllocated = 0;
	size_t totalPrimitiveSetsAllocated = 0;
	for(auto it=l.begin(); it!=l.end(); it++) {
		GeometryMemory* m = it->get();
		totalVerticesAllocated += m->vertexAllocationManager().allocated();
		totalIndicesAllocated += m->indexAllocationManager().allocated();
		totalPrimitiveSetsAllocated += m->primitiveSetAllocationManager().allocated();
	}

	// compute new capacities
	// - encourage minimal vertex allocation to be 1024 vertices; if a vertex (through its attributes) occupies about 36 bytes, this makes 36KiB
	// - encourage minimal index allocation to be 6*1024 indices; this makes 24KiB
	// - encourage minimal primitive set allocation to be 128; this makes 16*128 = 2KiB
	// All allocations together sums to 36+24+2 = 62KiB, e.g. we fit into 64KiB that is often the size of gpu memory page.
	size_t newVertexCapacity       = 2 * (totalVerticesAllocated      + numVertices);
	size_t newIndexCapacity        = 2 * (totalIndicesAllocated       + numIndices);
	size_t newPrimitiveSetCapacity = 2 * (totalPrimitiveSetsAllocated + numPrimitiveSets);
	if(newVertexCapacity       <   512)  newVertexCapacity       =   1024;
	if(newIndexCapacity        < 6*512)  newIndexCapacity        = 6*1024;
	if(newPrimitiveSetCapacity <    64)  newPrimitiveSetCapacity =    128;

	// alloc new GeometryMemory
	l.emplace_back(make_unique<GeometryMemory>(
		geometryStorage, newVertexCapacity, newIndexCapacity, newPrimitiveSetCapacity));
	GeometryMemory* m = l.back().get();
	if(m->tryAlloc(numVertices, numIndices, numPrimitiveSets,  // this should always succeed
	               _vertexDataID, _indexDataID, _primitiveSetDataID))
	{
		_geometryMemory = m;
		return;
	}
	throw runtime_error("Geometry::alloc(): Cannot alloc memory.");
}


void Geometry::realloc(size_t newNumVertices, size_t newNumIndices, size_t newNumPrimitiveSets)
{
	if(_geometryMemory->tryRealloc(newNumVertices, newNumIndices, newNumPrimitiveSets,
	                               _vertexDataID, _indexDataID, _primitiveSetDataID))
		return;

	throw runtime_error("Geometry::realloc(): Not implemented: Geometry object needs to be moved to another GeometryMemory.");
}


StagingBuffer& Geometry::createVertexStagingBuffer(size_t attribIndex)
{
	if(attribIndex >= _geometryMemory->numAttribs())
		throw std::out_of_range("Geometry::createVertexStagingBuffer() called with invalid attribIndex.");

	ArrayAllocation& a = vertexAllocation();
	const unsigned s = geometryStorage()->attribSizeList()[attribIndex];
	StagingManager& sm =
		StagingManager::getOrCreate(
			a,  // allocation
			_vertexDataID,  // allocationID
			_geometryMemory->vertexStagingManagerList(),  // stagingManagerList
			_geometryMemory->numAttribs()  // numBuffers
		);
	return
		sm.createStagingBuffer(
			attribIndex, // stagingBufferIndex
			renderer(),  // renderer
			_geometryMemory->buffer(),  // dstBuffer
			_geometryMemory->attribOffset(attribIndex)+(a.startIndex*s),  // dstOffset
			a.numItems*s  // size
		);
}


StagingBuffer& Geometry::createVertexSubsetStagingBuffer(size_t attribIndex, size_t firstVertex, size_t numVertices)
{
	if(attribIndex >= _geometryMemory->numAttribs())
		throw std::out_of_range("Geometry::createVertexSubsetStagingBuffer() called with invalid attribIndex.");

	ArrayAllocation& a = vertexAllocation();
	if(firstVertex+numVertices > a.numItems)
		throw std::out_of_range("Geometry::createVertexSubsetStagingBuffer(): firstVertex and numVertices arguments define range that is not completely inside allocated space of Geometry.");

	const unsigned s = geometryStorage()->attribSizeList()[attribIndex];
	StagingManager& sm =
		StagingManager::getOrCreateForSubsetUpdates(
			a,  // allocation
			_vertexDataID,  // allocationID
			_geometryMemory->vertexStagingManagerList()  // stagingManagerList
		);
	return
		sm.createStagingBuffer(
			renderer(),  // renderer
			_geometryMemory->buffer(),  // dstBuffer
			_geometryMemory->attribOffset(attribIndex)+((a.startIndex+firstVertex)*s),  // dstOffset
			numVertices*s  // size
		);
}


vector<StagingBuffer*> Geometry::createVertexStagingBuffers()
{
	vector<StagingBuffer*> v;
	size_t numAttribs = _geometryMemory->numAttribs();
	v.reserve(numAttribs);

	ArrayAllocation& a = vertexAllocation();
	StagingManager& sm = StagingManager::getOrCreate(
		a,  // allocation
		_vertexDataID,  // allocationID
		_geometryMemory->vertexStagingManagerList(),  // stagingManagerList
		numAttribs  // numBuffers
	);

	Renderer* r = renderer();
	vk::Buffer b = _geometryMemory->buffer();
	const auto& attribSizeList = geometryStorage()->attribSizeList();
	for(size_t i=0; i<numAttribs; i++) {
		const unsigned s = attribSizeList[i];
		StagingBuffer& sb =
			sm.createStagingBuffer(
				i,  // stagingBufferIndex
				r,  // renderer
				b,  // dstBuffer
				_geometryMemory->attribOffset(i)+(a.startIndex*s),  // dstOffset
				a.numItems*s  // size
			);
		v.emplace_back(&sb);
	}

	return v;
}


vector<StagingBuffer*> Geometry::createVertexSubsetStagingBuffers(size_t firstVertex,size_t numVertices)
{
	vector<StagingBuffer*> v;
	size_t numAttribs = _geometryMemory->numAttribs();
	v.reserve(numAttribs);

	ArrayAllocation& a = vertexAllocation();
	if(firstVertex+numVertices > a.numItems)
		throw std::out_of_range("Geometry::createVertexSubsetStagingBuffers(): firstVertex and numVertices arguments define range that is not completely inside allocated space of Geometry.");
	StagingManager& sm =
		StagingManager::getOrCreateForSubsetUpdates(
			a,  // allocation
			_vertexDataID,  // allocationID
			_geometryMemory->vertexStagingManagerList()  // stagingManagerList
		);

	Renderer* r = renderer();
	vk::Buffer b = _geometryMemory->buffer();
	const auto& attribSizeList = geometryStorage()->attribSizeList();
	for(size_t i=0; i<numAttribs; i++) {
		const unsigned s = attribSizeList[i];
		StagingBuffer& sb =
			sm.createStagingBuffer(
				r,  // renderer
				b,  // dstBuffer
				_geometryMemory->attribOffset(i)+((a.startIndex+firstVertex)*s),  // dstOffset
				numVertices*s  // size
			);
		v.emplace_back(&sb);
	}

	return v;
}


void Geometry::uploadVertexData(size_t attribIndex, size_t vertexCount, const void* ptr)
{
	// create StagingBuffer and submit it
	// (attribIndex bounds check is performed inside createVertexStagingBuffer())
	StagingBuffer& sb = createVertexStagingBuffer(attribIndex);
	size_t size = vertexCount * geometryStorage()->attribSizeList()[attribIndex];
	if(size != sb.size())
		throw std::out_of_range("Geometry::uploadVertexData(): size of attribData must match Geometry allocated space.");
	memcpy(sb.data(), ptr, size);
}


void Geometry::uploadVertexSubData(size_t attribIndex, size_t vertexCount, const void* ptr, size_t firstVertex)
{
	if(vertexCount == 0) return;

	// create StagingBuffer and submit it
	// (attribIndex bounds check is performed inside createVertexSubsetStagingBuffer(),
	// firstVertex and vertexCount validation is performed there too)
	StagingBuffer& sb = createVertexSubsetStagingBuffer(attribIndex, firstVertex, vertexCount);
	memcpy(sb.data(), ptr, vertexCount*geometryStorage()->attribSizeList()[attribIndex]);
}


void Geometry::uploadVertexData(size_t attributeCount, size_t vertexCount, const void*const* ptrList)
{
	// check parameters validity
	if(attributeCount != _geometryMemory->numAttribs())
		throw std::out_of_range("Geometry::uploadVertexData() called with invalid number of attributes.");
	if(vertexCount == 0)
		return;

	// create StagingBuffers and submit them
	const auto& attribSizeList = geometryStorage()->attribSizeList();
	for(size_t i=0; i<attributeCount; i++) {
		StagingBuffer& sb = createVertexStagingBuffer(i);
		memcpy(sb.data(), ptrList[i], vertexCount*attribSizeList[i]);
	}
}


void Geometry::uploadVertexSubData(size_t ptrListSize, size_t vertexCount, const void*const* ptrList, size_t firstVertex)
{
	// check parameters validity
	if(ptrListSize != _geometryMemory->numAttribs())
		throw std::out_of_range("Geometry::uploadVertexSubData() called with invalid number of attributes.");
	if(vertexCount == 0)
		return;

	// create StagingBuffers and submit them
	const auto& attribSizeList = geometryStorage()->attribSizeList();
	for(size_t i=0; i<ptrListSize; i++) {
		StagingBuffer& sb = createVertexSubsetStagingBuffer(i, firstVertex, vertexCount);
		memcpy(sb.data(), ptrList[i], vertexCount*attribSizeList[i]);
	}
}


StagingBuffer& Geometry::createIndexStagingBuffer()
{
	ArrayAllocation& a = indexAllocation();
	StagingManager& sm = StagingManager::getOrCreate(a, _indexDataID, _geometryMemory->indexStagingManagerList());
	return
		sm.createStagingBuffer(
			renderer(),  // renderer
			_geometryMemory->buffer(),  // dstBuffer
			_geometryMemory->indexOffset()+a.startIndex*sizeof(uint32_t),  // dstOffset
			a.numItems*sizeof(uint32_t)  // size
		);
}


StagingBuffer& Geometry::createIndexSubsetStagingBuffer(size_t firstIndex, size_t numIndices)
{
	ArrayAllocation& a = indexAllocation();
	if(firstIndex+numIndices > a.numItems)
		throw std::out_of_range("GeometryMemory::createIndexSubsetStagingBuffer(): Parameter firstIndex and numIndices define range that is not completely inside allocated space of Geometry.");
	StagingManager& sm = StagingManager::getOrCreate(a, _indexDataID, _geometryMemory->indexStagingManagerList());
	return
		sm.createStagingBuffer(
			renderer(),  // renderer
			_geometryMemory->buffer(),  // dstBuffer
			_geometryMemory->indexOffset()+(a.startIndex+firstIndex)*sizeof(uint32_t),  // dstOffset
			numIndices*sizeof(uint32_t)  // size
		);
}


void Geometry::uploadIndexData(size_t indexCount, const uint32_t* indices)
{
	if(indexCount == 0) return;

	// create StagingBuffer and submit it
	StagingBuffer& sb = createIndexStagingBuffer();
	if(indexCount*sizeof(uint32_t) != sb.size())
		throw std::out_of_range("GeometryMemory::uploadIndexData(): size of index data must match allocated space for indices inside Geometry.");
	memcpy(sb.data(), indices, sb.size());
}


void Geometry::uploadIndexSubData(size_t indexCount, const uint32_t* indices, size_t firstIndex)
{
	if(indexCount == 0) return;

	// create StagingBuffer and submit it
	StagingBuffer& sb = createIndexSubsetStagingBuffer(firstIndex, indexCount);
	memcpy(sb.data(), indices, sb.size());
}


StagingBuffer& Geometry::createPrimitiveSetStagingBuffer()
{
	ArrayAllocation& a = primitiveSetAllocation();
	StagingManager& sm = StagingManager::getOrCreate(a, _primitiveSetDataID, _geometryMemory->primitiveSetStagingManagerList());
	return
		sm.createStagingBuffer(
			renderer(),  // renderer
			_geometryMemory->buffer(),  // dstBuffer
			_geometryMemory->primitiveSetOffset()+a.startIndex*sizeof(PrimitiveSetGpuData),  // dstOffset
			a.numItems*sizeof(PrimitiveSetGpuData)  // size
		);
}


StagingBuffer& Geometry::createPrimitiveSetSubsetStagingBuffer(size_t firstPrimitiveSet, size_t numPrimitiveSets)
{
	ArrayAllocation& a = primitiveSetAllocation();
	if(firstPrimitiveSet+numPrimitiveSets > a.numItems)
		throw std::out_of_range("GeometryMemory::createPrimitiveSetSubsetStagingBuffer(): Parameter firstPrimitiveSet and numPrimitiveSets define range that is not completely inside allocated space of Geometry.");
	StagingManager& sm = StagingManager::getOrCreate(a, _primitiveSetDataID, _geometryMemory->primitiveSetStagingManagerList());
	return
		sm.createStagingBuffer(
			renderer(),  // renderer
			_geometryMemory->buffer(),  // dstBuffer
			_geometryMemory->primitiveSetOffset()+(a.startIndex+firstPrimitiveSet)*sizeof(PrimitiveSetGpuData),  // dstOffset
			numPrimitiveSets*sizeof(PrimitiveSetGpuData)  // size
		);
}


void Geometry::uploadPrimitiveSetData(size_t primitiveSetCount, const PrimitiveSetGpuData* psData)
{
	if(primitiveSetCount == 0) return;

	// create StagingBuffer and submit it
	StagingBuffer& sb = createPrimitiveSetStagingBuffer();
	if(primitiveSetCount*sizeof(PrimitiveSetGpuData) != sb.size())
		throw std::out_of_range("GeometryMemory::uploadPrimitiveSetData(): size of primitive set data data must match allocated space for primitive sets inside Geometry.");
	memcpy(sb.data(), psData, sb.size());
}


void Geometry::uploadPrimitiveSetSubData(size_t primitiveSetCount, const PrimitiveSetGpuData* psData, size_t firstPrimitiveSet)
{
	if(primitiveSetCount == 0) return;

	// create StagingBuffer and submit it
	StagingBuffer& sb = createPrimitiveSetSubsetStagingBuffer(firstPrimitiveSet, primitiveSetCount);
	memcpy(sb.data(), psData, sb.size());
}
