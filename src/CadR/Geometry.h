#pragma once

#include <CadR/Drawable.h>
#include <vector>

namespace CadR {

class AttribSizeList;
class AttribStorage;
class Renderer;
class StagingBuffer;
struct PrimitiveSetGpuData;
template<typename Owner> struct ArrayAllocation;


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

	AttribStorage* _attribStorage;     ///< AttribStorage where vertex and index data are stored.
	uint32_t _attribDataID = 0;        ///< ID of vertex data allocation inside AttribStorage.
	uint32_t _indexDataID = 0;         ///< ID of index data allocation inside AttribStorage.
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

	AttribStorage* attribStorage() const;
	Renderer* renderer() const;
	uint32_t attribDataID() const;
	uint32_t indexDataID() const;
	uint32_t primitiveSetDataID() const;

	void allocData(const AttribSizeList& attribSizeList,size_t numVertices,
	               size_t numIndices,size_t numPrimitiveSets);
	void allocData(size_t numVertices,size_t numIndices,size_t numPrimitiveSets);
	void reallocData(size_t numVertices,size_t numIndices,size_t numPrimitiveSets);
	void freeData();

	void allocAttribs(const AttribSizeList& attribSizeList,size_t num);
	void allocAttribs(size_t numVertices);
	void reallocAttribs(size_t numVertices);
	void freeAttribs();

	void allocIndices(size_t num);
	void reallocIndices(size_t num);
	void freeIndices();

	void allocPrimitiveSets(size_t num);
	void reallocPrimitiveSets(size_t num);
	void freePrimitiveSets();

	size_t numVertices() const;
	size_t numIndices() const;
	size_t numPrimitiveSets() const;

	const ArrayAllocation<Geometry>& attribAllocation() const; ///< Returns the attribute allocation.
	ArrayAllocation<Geometry>& attribAllocation();  ///< Returns the attribute allocation. Modify the returned data only with caution.
	const ArrayAllocation<Geometry>& indexAllocation() const;  ///< Returns the index allocation.
	ArrayAllocation<Geometry>& indexAllocation();   ///< Returns the index allocation. Modify the returned data only with caution.
	const ArrayAllocation<Geometry>& primitiveSetAllocation() const;  ///< Returns the primitiveSet allocation.
	ArrayAllocation<Geometry>& primitiveSetAllocation();   ///< Returns the primitiveSet allocation. Modify the returned data only with caution.

	void uploadAttribs(const std::vector<std::vector<uint8_t>>& vertexData,size_t dstIndex=0);
	void uploadAttrib(uint32_t attribIndex,const std::vector<uint8_t>& attribData,size_t dstIndex=0);
	StagingBuffer createStagingBuffer(uint32_t attribIndex);
	StagingBuffer createStagingBuffer(uint32_t attribIndex,size_t dstIndex,size_t numItems);
	std::vector<StagingBuffer> createStagingBuffers();
	std::vector<StagingBuffer> createStagingBuffers(size_t dstIndex,size_t numItems);

	void uploadIndices(std::vector<uint32_t>&& indexData,size_t dstIndex=0);
	StagingBuffer createIndexStagingBuffer();
	StagingBuffer createIndexStagingBuffer(size_t firstIndex,size_t numIndices);

	void uploadPrimitiveSets(std::vector<PrimitiveSetGpuData>&& primitiveSetData,size_t dstPrimitiveSetIndex=0);
	StagingBuffer createPrimitiveSetStagingBuffer();
	StagingBuffer createPrimitiveSetStagingBuffer(size_t firstPrimitiveSet,size_t numPrimitiveSets);

	friend Drawable;
};


}


// inline methods
#include <CadR/AttribStorage.h>
#include <CadR/Renderer.h>
#include <CadR/StagingBuffer.h>
namespace CadR {

inline Geometry::Geometry() : _attribStorage(Renderer::get()->emptyStorage())  {}
inline Geometry::Geometry(const AttribSizeList& attribSizeList,size_t numVertices,size_t numIndices,size_t numPrimitiveSets) : _attribStorage(Renderer::get()->emptyStorage())  { allocData(attribSizeList,numVertices,numIndices,numPrimitiveSets); }
inline Geometry::Geometry(Renderer* r) : _attribStorage(r->emptyStorage())  {}
inline Geometry::Geometry(Renderer* r,const AttribSizeList& attribSizeList,size_t numVertices,size_t numIndices,size_t numPrimitiveSets) : _attribStorage(r->emptyStorage())  { allocData(attribSizeList,numVertices,numIndices,numPrimitiveSets); }
inline Geometry::~Geometry()  { freeData(); }

inline AttribStorage* Geometry::attribStorage() const  { return _attribStorage; }
inline Renderer* Geometry::renderer() const  { return _attribStorage->renderer(); }
inline uint32_t Geometry::attribDataID() const  { return _attribDataID; }
inline uint32_t Geometry::indexDataID() const  { return _indexDataID; }
inline uint32_t Geometry::primitiveSetDataID() const  { return _primitiveSetDataID; }

inline void Geometry::allocData(const AttribSizeList& attribSizeList,size_t numVertices,size_t numIndices,size_t numPrimitiveSets)  { allocAttribs(attribSizeList,numVertices); allocIndices(numIndices); allocPrimitiveSets(numPrimitiveSets); }
inline void Geometry::allocData(size_t numVertices,size_t numIndices,size_t numPrimitiveSets)  { allocAttribs(numVertices); allocIndices(numIndices); allocPrimitiveSets(numPrimitiveSets); }
inline void Geometry::reallocData(size_t numVertices,size_t numIndices,size_t numPrimitiveSets)  { reallocAttribs(numVertices); reallocIndices(numIndices); reallocPrimitiveSets(numPrimitiveSets); }
inline void Geometry::freeData()  { freeAttribs(); freeIndices(); }

inline void Geometry::allocAttribs(const AttribSizeList& attribSizeList,size_t num)  { freeAttribs(); _attribStorage=renderer()->getOrCreateAttribStorage(attribSizeList); _attribDataID=_attribStorage->allocationManager().alloc(uint32_t(num),this); if(_attribDataID==0) throw std::runtime_error("Geometry: Failed to allocate attribute data."); }
inline void Geometry::allocAttribs(size_t num)  { freeAttribs(); _attribDataID=_attribStorage->allocationManager().alloc(uint32_t(num),this); }
inline void Geometry::reallocAttribs(size_t /*num*/)  { /* FIXME: not implemented yet */ }
inline void Geometry::freeAttribs()  { _attribStorage->allocationManager().free(_attribDataID); _attribDataID=0; }

inline void Geometry::allocIndices(size_t num)  { freeIndices(); _indexDataID=renderer()->indexAllocationManager().alloc(uint32_t(num),this); if(_indexDataID==0) throw std::runtime_error("Geometry: Failed to allocate index data."); }
inline void Geometry::reallocIndices(size_t /*num*/)  { /* FIXME: not implemented yet */ }
inline void Geometry::freeIndices()  { renderer()->indexAllocationManager().free(_indexDataID); _indexDataID=0; }

inline void Geometry::allocPrimitiveSets(size_t num)  { freePrimitiveSets(); _primitiveSetDataID=renderer()->primitiveSetAllocationManager().alloc(uint32_t(num),this); if(_primitiveSetDataID==0) throw std::runtime_error("Geometry: Failed to allocate primitiveSet data."); }
inline void Geometry::reallocPrimitiveSets(size_t /*num*/)  { /* FIXME: not implemented yet */ }
inline void Geometry::freePrimitiveSets()  { renderer()->primitiveSetAllocationManager().free(_primitiveSetDataID); _primitiveSetDataID=0; }

inline size_t Geometry::numVertices() const  { return size_t(_attribStorage->attribAllocation(_attribDataID).numItems); }
inline size_t Geometry::numIndices() const  { return size_t(renderer()->indexAllocation(_indexDataID).numItems); }
inline size_t Geometry::numPrimitiveSets() const  { return size_t(_attribStorage->renderer()->primitiveSetAllocation(_primitiveSetDataID).numItems); }

inline const ArrayAllocation<Geometry>& Geometry::attribAllocation() const  { return _attribStorage->attribAllocation(_attribDataID); }
inline ArrayAllocation<Geometry>& Geometry::attribAllocation()  { return _attribStorage->attribAllocation(_attribDataID); }
inline const ArrayAllocation<Geometry>& Geometry::indexAllocation() const  { return renderer()->indexAllocation(_indexDataID); }
inline ArrayAllocation<Geometry>& Geometry::indexAllocation()  { return renderer()->indexAllocation(_indexDataID); }
inline const ArrayAllocation<Geometry>& Geometry::primitiveSetAllocation() const  { return renderer()->primitiveSetAllocation(_primitiveSetDataID); }
inline ArrayAllocation<Geometry>& Geometry::primitiveSetAllocation()  { return renderer()->primitiveSetAllocation(_primitiveSetDataID); }

inline void Geometry::uploadAttribs(const std::vector<std::vector<uint8_t>>& vertexData,size_t dstIndex)  { _attribStorage->uploadAttribs(*this,vertexData,dstIndex); }
inline void Geometry::uploadAttrib(uint32_t attribIndex,const std::vector<uint8_t>& attribData,size_t dstIndex)  { _attribStorage->uploadAttrib(*this,attribIndex,attribData,dstIndex); }
inline StagingBuffer Geometry::createStagingBuffer(uint32_t attribIndex)  { return _attribStorage->createStagingBuffer(*this,attribIndex); }
inline StagingBuffer Geometry::createStagingBuffer(uint32_t attribIndex,size_t dstIndex,size_t numItems)  { return _attribStorage->createStagingBuffer(*this,attribIndex,dstIndex,numItems); }
inline std::vector<StagingBuffer> Geometry::createStagingBuffers()  { return _attribStorage->createStagingBuffers(*this); }
inline std::vector<StagingBuffer> Geometry::createStagingBuffers(size_t dstIndex,size_t numItems)  { return _attribStorage->createStagingBuffers(*this,dstIndex,numItems); }

inline void Geometry::uploadIndices(std::vector<uint32_t>&& indexData,size_t dstIndex)  { renderer()->uploadIndices(*this,std::move(indexData),dstIndex); }
inline StagingBuffer Geometry::createIndexStagingBuffer()  { return renderer()->createIndexStagingBuffer(*this); }
inline StagingBuffer Geometry::createIndexStagingBuffer(size_t firstIndex,size_t numIndices)  { return renderer()->createIndexStagingBuffer(*this,firstIndex,numIndices); }

inline void Geometry::uploadPrimitiveSets(std::vector<PrimitiveSetGpuData>&& primitiveSetData,size_t dstPrimitiveSetIndex)  { renderer()->uploadPrimitiveSets(*this,std::move(primitiveSetData),dstPrimitiveSetIndex); }
inline StagingBuffer Geometry::createPrimitiveSetStagingBuffer()  { return renderer()->createPrimitiveSetStagingBuffer(*this); }
inline StagingBuffer Geometry::createPrimitiveSetStagingBuffer(size_t firstPrimitiveSet,size_t numPrimitiveSets)  { return renderer()->createPrimitiveSetStagingBuffer(*this,firstPrimitiveSet,numPrimitiveSets); }

}
