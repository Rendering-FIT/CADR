#pragma once

#include <CadR/Drawable.h>
#include <vector>

namespace CadR {

class AttribSizeList;
class Renderer;
class StagingBuffer;
class VertexStorage;
struct ArrayAllocation;
struct PrimitiveSetGpuData;


/** Geometry class represents data used for rendering.
 *  It is composed of vertex data (vertex attributes), indices (optionally) and primitiveSets.
 *
 *  CADR is designed to handle very large number of Geometry objects rendered in real-time.
 *  Hundreds of thousands should be reachable on nowadays hi-end systems.
 *  This is important for many CAD applications, as CAD scenes are often composed
 *  of high number of small parts of various detail.
 *
 *  DrawCommand class is used to render Geometry using particular StateSet
 *  and instanced by evaluating Transformation graph.
 */
class CADR_EXPORT Geometry final {
protected:

	VertexStorage* _vertexStorage;     ///< Pointer to VertexStorage where vertex attributes are stored.
	uint32_t _vertexDataID = 0;        ///< ID of vertex data allocation inside VertexStorage.
	uint32_t _indexDataID = 0;         ///< ID of index data allocation inside Renderer.
	uint32_t _primitiveSetDataID = 0;  ///< ID of DrawCommand data allocation.
	DrawableList _drawableList;        ///< List of all Drawables referencing this Geometry.

public:

	Geometry();  ///< Default constructor. It does not allocate or reserve any memory. Renderer is initialized to use default Renderer returned by Renderer::get().
	Geometry(Renderer* r);  ///< Constructs the object without allocating or reserving any memory.
	Geometry(const AttribSizeList& attribSizeList,size_t numVertices,
	         size_t numIndices,size_t numPrimitiveSets);  ///< Constructs the object and allocate memory. Geometry will use default Renderer returned by Renderer::get().
	Geometry(Renderer* r,const AttribSizeList& attribSizeList,size_t numVertices,
	         size_t numIndices,size_t numPrimitiveSets);  ///< Constructs the object and allocate memory.
	Geometry(const Geometry&) = delete;  ///< No copy constructor. Only move contructors are allowed.
	Geometry(Geometry&& g);  ///< Move constructor.
	~Geometry();  ///< Destructor.

	Geometry& operator=(const Geometry&) = delete;  ///< No assignment operator. Only move assignment is allowed.
	Geometry& operator=(Geometry&& rhs);  ///< Move operator.

	VertexStorage* vertexStorage() const;
	Renderer* renderer() const;
	uint32_t vertexDataID() const;
	uint32_t indexDataID() const;
	uint32_t primitiveSetDataID() const;

	void alloc(const AttribSizeList& attribSizeList,size_t numVertices,
	           size_t numIndices,size_t numPrimitiveSets);
	void alloc(size_t numVertices,size_t numIndices,size_t numPrimitiveSets);
	void realloc(size_t numVertices,size_t numIndices,size_t numPrimitiveSets);
	void free();

	void allocVertices(const AttribSizeList& attribSizeList,size_t num);
	void allocVertices(size_t num);
	void reallocVertices(size_t num);
	void freeVertices();

	void allocIndices(size_t num);
	void reallocIndices(size_t num);
	void freeIndices();

	void allocPrimitiveSets(size_t num);
	void reallocPrimitiveSets(size_t num);
	void freePrimitiveSets();

	size_t numVertices() const;
	size_t numIndices() const;
	size_t numPrimitiveSets() const;

	const ArrayAllocation& vertexAllocation() const; ///< Returns the vertex allocation.
	ArrayAllocation& vertexAllocation();  ///< Returns the vertex allocation. Modify the returned data only with caution.
	const ArrayAllocation& indexAllocation() const;  ///< Returns the index allocation.
	ArrayAllocation& indexAllocation();   ///< Returns the index allocation. Modify the returned data only with caution.
	const ArrayAllocation& primitiveSetAllocation() const;  ///< Returns the primitiveSet allocation.
	ArrayAllocation& primitiveSetAllocation();   ///< Returns the primitiveSet allocation. Modify the returned data only with caution.

	void uploadVertices(const std::vector<std::vector<uint8_t>>& vertexData);
	void uploadVertices(uint32_t attribIndex,const std::vector<uint8_t>& attribData);
	void uploadVerticesSubset(const std::vector<std::vector<uint8_t>>& vertexData,size_t firstVertex);
	void uploadVerticesSubset(uint32_t attribIndex,const std::vector<uint8_t>& attribData,size_t firstVertex);
	StagingBuffer& createStagingBuffer(uint32_t attribIndex);
	StagingBuffer& createSubsetStagingBuffer(uint32_t attribIndex,size_t firstVertex,size_t numVertices);
	std::vector<StagingBuffer*> createStagingBuffers();
	std::vector<StagingBuffer*> createSubsetStagingBuffers(size_t firstVertex,size_t numVertices);
	StagingManager* vertexStagingManager();
	const StagingManager* vertexStagingManager() const;

	void uploadIndices(std::vector<uint32_t>& indexData);
	void uploadIndicesSubset(std::vector<uint32_t>& indexData,size_t dstIndex);
	StagingBuffer& createIndexStagingBuffer();
	StagingBuffer& createIndexStagingSubBuffer(size_t firstIndex,size_t numIndices);
	StagingManager* indexStagingManager();
	const StagingManager* indexStagingManager() const;

	void uploadPrimitiveSets(std::vector<PrimitiveSetGpuData>& primitiveSetData);
	void uploadPrimitiveSetsSubset(std::vector<PrimitiveSetGpuData>& primitiveSetData,size_t firstPrimitiveSetIndex);
	StagingBuffer& createPrimitiveSetStagingBuffer();
	StagingBuffer& createPrimitiveSetSubsetStagingBuffer(size_t firstPrimitiveSet,size_t numPrimitiveSets);

	friend Drawable;
};


}


// inline methods
#include <CadR/Renderer.h>
#include <CadR/StagingBuffer.h>
#include <CadR/VertexStorage.h>
namespace CadR {

inline Geometry::Geometry() : _vertexStorage(Renderer::get()->emptyStorage())  {}
inline Geometry::Geometry(const AttribSizeList& attribSizeList,size_t numVertices,size_t numIndices,size_t numPrimitiveSets) : _vertexStorage(Renderer::get()->emptyStorage())  { alloc(attribSizeList,numVertices,numIndices,numPrimitiveSets); }
inline Geometry::Geometry(Renderer* r) : _vertexStorage(r->emptyStorage())  {}
inline Geometry::Geometry(Renderer* r,const AttribSizeList& attribSizeList,size_t numVertices,size_t numIndices,size_t numPrimitiveSets) : _vertexStorage(r->emptyStorage())  { alloc(attribSizeList,numVertices,numIndices,numPrimitiveSets); }
inline Geometry::~Geometry()  { free(); }

inline VertexStorage* Geometry::vertexStorage() const  { return _vertexStorage; }
inline Renderer* Geometry::renderer() const  { return _vertexStorage->renderer(); }
inline uint32_t Geometry::vertexDataID() const  { return _vertexDataID; }
inline uint32_t Geometry::indexDataID() const  { return _indexDataID; }
inline uint32_t Geometry::primitiveSetDataID() const  { return _primitiveSetDataID; }

inline void Geometry::alloc(const AttribSizeList& attribSizeList,size_t numVertices,size_t numIndices,size_t numPrimitiveSets)  { allocVertices(attribSizeList,numVertices); allocIndices(numIndices); allocPrimitiveSets(numPrimitiveSets); }
inline void Geometry::alloc(size_t numVertices,size_t numIndices,size_t numPrimitiveSets)  { allocVertices(numVertices); allocIndices(numIndices); allocPrimitiveSets(numPrimitiveSets); }
inline void Geometry::realloc(size_t numVertices,size_t numIndices,size_t numPrimitiveSets)  { reallocVertices(numVertices); reallocIndices(numIndices); reallocPrimitiveSets(numPrimitiveSets); }
inline void Geometry::free()  { freeVertices(); freeIndices(); }

inline void Geometry::allocVertices(const AttribSizeList& attribSizeList,size_t num)  { freeVertices(); _vertexStorage=renderer()->getOrCreateVertexStorage(attribSizeList); _vertexDataID=_vertexStorage->allocationManager().alloc(uint32_t(num),this); if(_vertexDataID==0) throw std::runtime_error("Geometry: Failed to allocate vertex data."); }
inline void Geometry::allocVertices(size_t num)  { freeVertices(); _vertexDataID=_vertexStorage->allocationManager().alloc(uint32_t(num),this); }
inline void Geometry::reallocVertices(size_t /*num*/)  { /* FIXME: not implemented yet */ }
inline void Geometry::freeVertices()  { _vertexStorage->allocationManager().free(_vertexDataID); _vertexDataID=0; }

inline void Geometry::allocIndices(size_t num)  { freeIndices(); _indexDataID=renderer()->indexAllocationManager().alloc(uint32_t(num),this); if(_indexDataID==0) throw std::runtime_error("Geometry: Failed to allocate index data."); }
inline void Geometry::reallocIndices(size_t /*num*/)  { /* FIXME: not implemented yet */ }
inline void Geometry::freeIndices()  { renderer()->indexAllocationManager().free(_indexDataID); _indexDataID=0; }

inline void Geometry::allocPrimitiveSets(size_t num)  { freePrimitiveSets(); _primitiveSetDataID=renderer()->primitiveSetAllocationManager().alloc(uint32_t(num),this); if(_primitiveSetDataID==0) throw std::runtime_error("Geometry: Failed to allocate primitiveSet data."); }
inline void Geometry::reallocPrimitiveSets(size_t /*num*/)  { /* FIXME: not implemented yet */ }
inline void Geometry::freePrimitiveSets()  { renderer()->primitiveSetAllocationManager().free(_primitiveSetDataID); _primitiveSetDataID=0; }

inline size_t Geometry::numVertices() const  { return size_t(_vertexStorage->allocation(*this).numItems); }
inline size_t Geometry::numIndices() const  { return size_t(renderer()->indexAllocation(*this).numItems); }
inline size_t Geometry::numPrimitiveSets() const  { return size_t(_vertexStorage->renderer()->primitiveSetAllocation(*this).numItems); }

inline const ArrayAllocation& Geometry::vertexAllocation() const  { return _vertexStorage->allocation(*this); }
inline ArrayAllocation& Geometry::vertexAllocation()  { return _vertexStorage->allocation(*this); }
inline const ArrayAllocation& Geometry::indexAllocation() const  { return renderer()->indexAllocation(*this); }
inline ArrayAllocation& Geometry::indexAllocation()  { return renderer()->indexAllocation(*this); }
inline const ArrayAllocation& Geometry::primitiveSetAllocation() const  { return renderer()->primitiveSetAllocation(*this); }
inline ArrayAllocation& Geometry::primitiveSetAllocation()  { return renderer()->primitiveSetAllocation(*this); }

inline void Geometry::uploadVertices(const std::vector<std::vector<uint8_t>>& vertexData)  { _vertexStorage->upload(*this,vertexData); }
inline void Geometry::uploadVertices(uint32_t attribIndex,const std::vector<uint8_t>& attribData)  { _vertexStorage->upload(*this,attribIndex,attribData); }
inline void Geometry::uploadVerticesSubset(const std::vector<std::vector<uint8_t>>& vertexData,size_t firstVertex)  { _vertexStorage->uploadSubset(*this,vertexData,firstVertex); }
inline void Geometry::uploadVerticesSubset(uint32_t attribIndex,const std::vector<uint8_t>& attribData,size_t firstVertex)  { _vertexStorage->uploadSubset(*this,attribIndex,attribData,firstVertex); }
inline StagingBuffer& Geometry::createStagingBuffer(uint32_t attribIndex)  { return _vertexStorage->createStagingBuffer(*this,attribIndex); }
inline StagingBuffer& Geometry::createSubsetStagingBuffer(uint32_t attribIndex,size_t firstVertex,size_t numVertices)  { return _vertexStorage->createSubsetStagingBuffer(*this,attribIndex,firstVertex,numVertices); }
inline std::vector<StagingBuffer*> Geometry::createStagingBuffers()  { return move(_vertexStorage->createStagingBuffers(*this)); }
inline std::vector<StagingBuffer*> Geometry::createSubsetStagingBuffers(size_t firstVertex,size_t numVertices)  { return move(_vertexStorage->createSubsetStagingBuffers(*this,firstVertex,numVertices)); }
inline StagingManager* Geometry::vertexStagingManager()  { return _vertexStorage->allocation(*this).stagingManager; }
inline const StagingManager* Geometry::vertexStagingManager() const  { return _vertexStorage->allocation(*this).stagingManager; }

inline void Geometry::uploadIndices(std::vector<uint32_t>& indexData)  { renderer()->uploadIndices(*this,indexData); }
inline void Geometry::uploadIndicesSubset(std::vector<uint32_t>& indexData,size_t dstIndex)  { renderer()->uploadIndicesSubset(*this,indexData,dstIndex); }
inline StagingBuffer& Geometry::createIndexStagingBuffer()  { return renderer()->createIndexStagingBuffer(*this); }
inline StagingBuffer& Geometry::createIndexStagingSubBuffer(size_t firstIndex,size_t numIndices)  { return renderer()->createIndexSubsetStagingBuffer(*this,firstIndex,numIndices); }
inline StagingManager* Geometry::indexStagingManager()  { return indexAllocation().stagingManager; }
inline const StagingManager* Geometry::indexStagingManager() const  { return indexAllocation().stagingManager; }

inline void Geometry::uploadPrimitiveSets(std::vector<PrimitiveSetGpuData>& primitiveSetData)  { renderer()->uploadPrimitiveSets(*this,primitiveSetData); }
inline void Geometry::uploadPrimitiveSetsSubset(std::vector<PrimitiveSetGpuData>& primitiveSetData,size_t firstPrimitiveSetIndex)  { renderer()->uploadPrimitiveSetsSubset(*this,primitiveSetData,firstPrimitiveSetIndex); }
inline StagingBuffer& Geometry::createPrimitiveSetStagingBuffer()  { return renderer()->createPrimitiveSetStagingBuffer(*this); }
inline StagingBuffer& Geometry::createPrimitiveSetSubsetStagingBuffer(size_t firstPrimitiveSet,size_t numPrimitiveSets)  { return renderer()->createPrimitiveSetSubsetStagingBuffer(*this,firstPrimitiveSet,numPrimitiveSets); }

// extra methods are put here to avoid nightmare of circular includes
inline const ArrayAllocation& Renderer::indexAllocation(const Geometry& g) const  { return _indexAllocationManager[g.indexDataID()]; }
inline ArrayAllocation& Renderer::indexAllocation(Geometry& g)  { return _indexAllocationManager[g.indexDataID()]; }
inline const ArrayAllocation& Renderer::primitiveSetAllocation(const Geometry& g) const  { return _primitiveSetAllocationManager[g.primitiveSetDataID()]; }
inline ArrayAllocation& Renderer::primitiveSetAllocation(Geometry& g)  { return _primitiveSetAllocationManager[g.primitiveSetDataID()]; }


}
