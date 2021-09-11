#pragma once

#include <CadR/Drawable.h>
#include <vector>

namespace CadR {

class AttribSizeList;
class GeometryMemory;
class Renderer;
class StagingBuffer;
struct ArrayAllocation;
struct PrimitiveSetGpuData;


/** Geometry class represents data used for rendering.
 *  It is composed of vertex data (vertex attributes), index data and primitiveSets.
 *
 *  CADR is designed to handle very large number of Geometry objects rendered in real-time.
 *  Hundreds of thousands geometries should be reachable on nowadays hi-end systems.
 *  This is important for many CAD applications, as CAD scenes are often composed
 *  of high number of small parts of various detail.
 *
 *  Drawable class is used to render Geometry using particular StateSet
 *  and instanced by transformation list.
 */
class CADR_EXPORT Geometry final {
protected:

	GeometryMemory* _geometryMemory;   ///< Pointer to VertexMemory and indirectly to VertexStorage where vertex attributes are stored.
	uint32_t _vertexDataID = 0;        ///< ID of vertex data allocation inside VertexStorage.
	uint32_t _indexDataID = 0;         ///< ID of index data allocation inside Renderer.
	uint32_t _primitiveSetDataID = 0;  ///< ID of PrimitiveSet data allocation.
	DrawableList _drawableList;        ///< List of all Drawables referencing this Geometry.

public:

	// construction and destruction
	Geometry();  ///< Default constructor. It does not allocate or reserve any memory. The object is initialized to use default Renderer returned by Renderer::get().
	Geometry(Renderer* r);  ///< Constructs the object without allocating or reserving any memory.
	Geometry(const AttribSizeList& attribSizeList, size_t numVertices,
	         size_t numIndices, size_t numPrimitiveSets);  ///< Constructs the object and allocate memory. Geometry will use default Renderer returned by Renderer::get().
	Geometry(Renderer* r, const AttribSizeList& attribSizeList, size_t numVertices,
	         size_t numIndices, size_t numPrimitiveSets);  ///< Constructs the object and allocate memory.
	Geometry(const Geometry&) = delete;  ///< No copy constructor. Only move contructors are allowed.
	Geometry(Geometry&& g);  ///< Move constructor.
	~Geometry();  ///< Destructor.

	// operators
	Geometry& operator=(const Geometry&) = delete;  ///< No assignment operator. Only move assignment is allowed.
	Geometry& operator=(Geometry&& rhs);  ///< Move operator.

	// getters
	GeometryMemory* geometryMemory() const;
	GeometryStorage* geometryStorage() const;
	Renderer* renderer() const;
	uint32_t vertexDataID() const;
	uint32_t indexDataID() const;
	uint32_t primitiveSetDataID() const;

	size_t numVertices() const;
	size_t numIndices() const;
	size_t numPrimitiveSets() const;

	// memory management
	void alloc(const AttribSizeList& attribSizeList, size_t numVertices,
	           size_t numIndices, size_t numPrimitiveSets);
	void alloc(GeometryStorage* geometryStorage, size_t numVertices,
	           size_t numIndices, size_t numPrimitiveSets);
	void alloc(size_t numVertices, size_t numIndices, size_t numPrimitiveSets);
	void realloc(size_t numVertices, size_t numIndices, size_t numPrimitiveSets);
	void free();

	// allocation structures
	ArrayAllocation& vertexAllocation();  ///< Returns the vertex allocation. Modify the returned data only with caution.
	const ArrayAllocation& vertexAllocation() const; ///< Returns the vertex allocation.
	ArrayAllocation& indexAllocation();   ///< Returns the index allocation. Modify the returned data only with caution.
	const ArrayAllocation& indexAllocation() const;  ///< Returns the index allocation.
	ArrayAllocation& primitiveSetAllocation();   ///< Returns the primitiveSet allocation. Modify the returned data only with caution.
	const ArrayAllocation& primitiveSetAllocation() const;  ///< Returns the primitiveSet allocation.

	// data upload
	void uploadVertexData(size_t ptrListSize, size_t vertexCount, const void*const* ptrList);
	void uploadVertexData(size_t attribIndex, size_t vertexCount, const void* ptr);
	void uploadIndexData (size_t indexCount,  const uint32_t* indices);
	void uploadPrimitiveSetData(size_t primitiveSetCount, const PrimitiveSetGpuData* psData);
	StagingBuffer& createVertexStagingBuffer(size_t attribIndex);
	StagingBuffer& createIndexStagingBuffer();
	StagingBuffer& createPrimitiveSetStagingBuffer();
	std::vector<StagingBuffer*> createVertexStagingBuffers();

	// subdata upload
	void uploadVertexSubData(size_t ptrListSize, size_t vertexCount, const void*const* ptrList, size_t firstVertex);
	void uploadVertexSubData(size_t attribIndex, size_t vertexCount, const void* ptr, size_t firstVertex);
	void uploadIndexSubData (size_t indexCount,  const uint32_t* indices, size_t firstIndex);
	void uploadPrimitiveSetSubData(size_t primitiveSetCount, const PrimitiveSetGpuData* psData, size_t firstPrimitiveSet);
	StagingBuffer& createVertexSubsetStagingBuffer(size_t attribIndex, size_t firstVertex, size_t numVertices);
	StagingBuffer& createIndexSubsetStagingBuffer (size_t firstIndex,  size_t numIndices);
	StagingBuffer& createPrimitiveSetSubsetStagingBuffer(size_t firstPrimitiveSet, size_t numPrimitiveSets);
	std::vector<StagingBuffer*> createVertexSubsetStagingBuffers(size_t firstVertex, size_t numVertices);

	friend Drawable;
	friend GeometryMemory;
};


}


// inline methods
#include <CadR/Renderer.h>
#include <CadR/GeometryMemory.h>
#include <CadR/GeometryStorage.h>
namespace CadR {

inline Geometry::Geometry() : _geometryMemory(Renderer::get()->emptyGeometryMemory())  {}
inline Geometry::Geometry(const AttribSizeList& attribSizeList, size_t numVertices, size_t numIndices, size_t numPrimitiveSets) : _geometryMemory(Renderer::get()->emptyGeometryMemory())  { alloc(attribSizeList, numVertices, numIndices, numPrimitiveSets); }
inline Geometry::Geometry(Renderer* r) : _geometryMemory(r->emptyGeometryMemory())  {}
inline Geometry::Geometry(Renderer* r, const AttribSizeList& attribSizeList, size_t numVertices, size_t numIndices, size_t numPrimitiveSets) : _geometryMemory(r->emptyGeometryMemory())  { alloc(attribSizeList, numVertices, numIndices, numPrimitiveSets); }
inline Geometry::~Geometry()  { free(); }

inline GeometryMemory* Geometry::geometryMemory() const  { return _geometryMemory; }
inline GeometryStorage* Geometry::geometryStorage() const  { return _geometryMemory->geometryStorage(); }
inline Renderer* Geometry::renderer() const  { return _geometryMemory->geometryStorage()->renderer(); }
inline uint32_t Geometry::vertexDataID() const  { return _vertexDataID; }
inline uint32_t Geometry::indexDataID() const  { return _indexDataID; }
inline uint32_t Geometry::primitiveSetDataID() const  { return _primitiveSetDataID; }

inline size_t Geometry::numVertices() const  { return size_t(vertexAllocation().numItems); }
inline size_t Geometry::numIndices() const  { return size_t(indexAllocation().numItems); }
inline size_t Geometry::numPrimitiveSets() const  { return size_t(primitiveSetAllocation().numItems); }

inline void Geometry::alloc(const AttribSizeList& attribSizeList, size_t numVertices, size_t numIndices, size_t numPrimitiveSets)  { alloc(renderer()->getOrCreateGeometryStorage(attribSizeList), numVertices, numIndices, numPrimitiveSets); }
inline void Geometry::alloc(size_t numVertices, size_t numIndices, size_t numPrimitiveSets)  { alloc(geometryStorage(), numVertices, numIndices, numPrimitiveSets); }
inline void Geometry::free() {
	if(_vertexDataID!=0 || _indexDataID!=0 || _primitiveSetDataID!=0) { 
		_geometryMemory->free(_vertexDataID, _indexDataID, _primitiveSetDataID);
		_vertexDataID = 0; _indexDataID = 0; _primitiveSetDataID = 0;
	}
}

inline ArrayAllocation& Geometry::vertexAllocation()  { return _geometryMemory->vertexAllocationManager()[_vertexDataID]; }
inline const ArrayAllocation& Geometry::vertexAllocation() const  { return _geometryMemory->vertexAllocationManager()[_vertexDataID]; }
inline ArrayAllocation& Geometry::indexAllocation()  { return _geometryMemory->indexAllocationManager()[_indexDataID]; }
inline const ArrayAllocation& Geometry::indexAllocation() const  { return _geometryMemory->indexAllocationManager()[_indexDataID]; }
inline ArrayAllocation& Geometry::primitiveSetAllocation()  { return _geometryMemory->primitiveSetAllocationManager()[_primitiveSetDataID]; }
inline const ArrayAllocation& Geometry::primitiveSetAllocation() const  { return _geometryMemory->primitiveSetAllocationManager()[_primitiveSetDataID]; }


}
