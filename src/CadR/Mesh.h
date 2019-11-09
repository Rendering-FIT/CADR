#pragma once

#include <vector>
#include <CadR/Drawable.h>
#include <CadR/Export.h>

namespace CadR {

class AttribSizeList;
class AttribStorage;
class Renderer;
class StagingBuffer;
struct PrimitiveSetGpuData;
template<typename Owner> struct ArrayAllocation;


/** Mesh class represents geometry data that might be used for rendering.
 *  It is composed of vertex data (vertex attributes), indices (optionally) and primitiveSets.
 *
 *  CADR is designed to handle very large number of Mesh instances rendered in real-time.
 *  Hundreds of thousands should be reachable on nowadays hi-end systems.
 *  This is important for many CAD applications, when scenes are often composed
 *  of high number of small parts of various detail.
 *
 *  DrawCmd class is used to render Meshes using particular StateSet and instancing them by
 *  evaluating Transformation graph.
 */
class CADR_EXPORT Mesh final {
protected:

	AttribStorage* _attribStorage;  ///< AttribStorage where vertex and index data are stored.
	uint32_t _attribDataID = 0;  ///< ID of vertex data allocation inside AttribStorage.
	uint32_t _indexDataID = 0;  ///< ID of index data allocation inside AttribStorage.
	uint32_t _primitiveSetDataID = 0;  ///< ID od DrawCommand data allocation.
	DrawableList _drawableList;

public:

	Mesh();  ///< Default constructor. It does not allocate or reserve any memory. Renderer is initialized to use default Renderer returned by Renderer::get().
	Mesh(Renderer* r);  ///< Constructs the object without allocating or reserving any memory.
	Mesh(const AttribSizeList& attribSizeList,size_t numVertices,
	     size_t numIndices,size_t numPrimitiveSets);  ///< Constructs the object and allocate memory. Mesh will use default Renderer returned by Renderer::get().
	Mesh(Renderer* r,const AttribSizeList& attribSizeList,size_t numVertices,
	     size_t numIndices,size_t numPrimitiveSets);  ///< Constructs the object and allocate memory.
	Mesh(const Mesh&) = delete;  ///< No copy constructor. Only move contructors are allowed.
	Mesh(Mesh&& m);  ///< Move constructor.
	~Mesh();  ///< Destructor.

	Mesh& operator=(const Mesh&) = delete;  ///< No assignment operator. Only move assignment is allowed.
	Mesh& operator=(Mesh&& rhs);  ///< Move operator.

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

	const ArrayAllocation<Mesh>& attribAllocation() const; ///< Returns the attribute allocation.
	ArrayAllocation<Mesh>& attribAllocation();  ///< Returns the attribute allocation. Modify the returned data only with caution.
	const ArrayAllocation<Mesh>& indexAllocation() const;  ///< Returns the index allocation.
	ArrayAllocation<Mesh>& indexAllocation();   ///< Returns the index allocation. Modify the returned data only with caution.
	const ArrayAllocation<Mesh>& primitiveSetAllocation() const;  ///< Returns the primitiveSet allocation.
	ArrayAllocation<Mesh>& primitiveSetAllocation();   ///< Returns the primitiveSet allocation. Modify the returned data only with caution.

	void uploadAttribs(const std::vector<std::vector<uint8_t>>& vertexData,size_t dstIndex=0);
	void uploadAttrib(uint32_t attribIndex,const std::vector<uint8_t>& attribData,size_t dstIndex=0);
	StagingBuffer createStagingBuffer(uint32_t attribIndex);
	StagingBuffer createStagingBuffer(uint32_t attribIndex,size_t dstIndex,size_t numItems);
	std::vector<StagingBuffer> createStagingBuffers();
	std::vector<StagingBuffer> createStagingBuffers(size_t dstIndex,size_t numItems);

	void uploadIndices(std::vector<uint32_t>&& indexData,size_t dstIndex=0);
	StagingBuffer createIndexStagingBuffer();
	StagingBuffer createIndexStagingBuffer(size_t firstIndex,size_t numIndices);

	void uploadPrimitiveSets(std::vector<PrimitiveSetGpuData>&& primitiveSetData,size_t dstPrimitiveSet);
	StagingBuffer createPrimitiveSetStagingBuffer();
	StagingBuffer createPrimitiveSetStagingBuffer(size_t firstPrimitiveSet,size_t numPrimitiveSets);

#if 0
	inline void uploadDrawCommands(const std::vector<DrawCommand>&& drawCommands,
	                               size_t dstIndex=0);

         inline void uploadPrimitives(const PrimitiveGpuData *bufferData,
                                      uint32_t numPrimitives,uint32_t dstIndex=0);
         inline void setPrimitives(const Primitive *primitiveList,
                                   uint32_t numPrimitives,uint32_t startIndex=0,
                                   bool truncate=true);
         inline void setAndUploadPrimitives(PrimitiveGpuData *nonConstBufferData,
                                            const Primitive *primitiveList,uint32_t numPrimitives);
         inline void setAndUploadPrimitives(PrimitiveGpuData *nonConstBufferData,
                                            const uint32_t *modesAndOffsets4,uint32_t numPrimitives);
         inline void updateVertexOffsets(void *primitiveBuffer,
                                         const Primitive *primitiveList,uint32_t numPrimitives);
         inline static std::vector<Primitive> generatePrimitiveList(
                                         const uint32_t *modesAndOffsets4,uint32_t numPrimitives);

         inline void clearPrimitives();
         inline void setNumPrimitives(uint32_t num);

	inline DrawableId createGeode(MatrixList *matrixList,StateSet *stateSet);
	inline DrawableId createGeode(const uint32_t *primitiveIndices,
	                              const uint32_t primitiveCount,
	                              MatrixList *matrixList,StateSet *stateSet);
	inline void deleteGeode(DrawableId id);
#endif

};


}


// inline methods
#include <CadR/AttribStorage.h>
#include <CadR/Renderer.h>
#include <CadR/StagingBuffer.h>
namespace CadR {

inline Mesh::Mesh() : _attribStorage(Renderer::get()->emptyStorage())  {}
inline Mesh::Mesh(const AttribSizeList& attribSizeList,size_t numVertices,size_t numIndices,size_t numPrimitiveSets) : _attribStorage(Renderer::get()->emptyStorage())  { allocData(attribSizeList,numVertices,numIndices,numPrimitiveSets); }
inline Mesh::Mesh(Renderer* r) : _attribStorage(r->emptyStorage())  {}
inline Mesh::Mesh(Renderer* r,const AttribSizeList& attribSizeList,size_t numVertices,size_t numIndices,size_t numPrimitiveSets) : _attribStorage(r->emptyStorage())  { allocData(attribSizeList,numVertices,numIndices,numPrimitiveSets); }
inline Mesh::~Mesh()  { freeData(); }

inline AttribStorage* Mesh::attribStorage() const  { return _attribStorage; }
inline Renderer* Mesh::renderer() const  { return _attribStorage->renderer(); }
inline uint32_t Mesh::attribDataID() const  { return _attribDataID; }
inline uint32_t Mesh::indexDataID() const  { return _indexDataID; }
inline uint32_t Mesh::primitiveSetDataID() const  { return _primitiveSetDataID; }

inline void Mesh::allocData(const AttribSizeList& attribSizeList,size_t numVertices,size_t numIndices,size_t numPrimitiveSets)  { allocAttribs(attribSizeList,numVertices); allocIndices(numIndices); allocPrimitiveSets(numPrimitiveSets); }
inline void Mesh::allocData(size_t numVertices,size_t numIndices,size_t numPrimitiveSets)  { allocAttribs(numVertices); allocIndices(numIndices); allocPrimitiveSets(numPrimitiveSets); }
inline void Mesh::reallocData(size_t numVertices,size_t numIndices,size_t numPrimitiveSets)  { reallocAttribs(numVertices); reallocIndices(numIndices); reallocPrimitiveSets(numPrimitiveSets); }
inline void Mesh::freeData()  { freeAttribs(); freeIndices(); }

inline void Mesh::allocAttribs(const AttribSizeList& attribSizeList,size_t num)  { freeAttribs(); _attribStorage=renderer()->getOrCreateAttribStorage(attribSizeList); _attribDataID=_attribStorage->allocationManager().alloc(uint32_t(num),this); if(_attribDataID==0) throw std::runtime_error("Mesh: Failed to allocate attribute data."); }
inline void Mesh::allocAttribs(size_t num)  { freeAttribs(); _attribDataID=_attribStorage->allocationManager().alloc(uint32_t(num),this); }
inline void Mesh::reallocAttribs(size_t /*num*/)  { /* FIXME: not implemented yet */ }
inline void Mesh::freeAttribs()  { _attribStorage->allocationManager().free(_attribDataID); _attribDataID=0; }

inline void Mesh::allocIndices(size_t num)  { freeIndices(); _indexDataID=renderer()->indexAllocationManager().alloc(uint32_t(num),this); if(_indexDataID==0) throw std::runtime_error("Mesh: Failed to allocate index data."); }
inline void Mesh::reallocIndices(size_t /*num*/)  { /* FIXME: not implemented yet */ }
inline void Mesh::freeIndices()  { renderer()->indexAllocationManager().free(_indexDataID); _indexDataID=0; }

inline void Mesh::allocPrimitiveSets(size_t num)  { freePrimitiveSets(); _primitiveSetDataID=renderer()->primitiveSetAllocationManager().alloc(uint32_t(num),this); if(_primitiveSetDataID==0) throw std::runtime_error("Mesh: Failed to allocate primitiveSet data."); }
inline void Mesh::reallocPrimitiveSets(size_t /*num*/)  { /* FIXME: not implemented yet */ }
inline void Mesh::freePrimitiveSets()  { renderer()->primitiveSetAllocationManager().free(_primitiveSetDataID); _primitiveSetDataID=0; }

inline size_t Mesh::numVertices() const  { return size_t(_attribStorage->attribAllocation(_attribDataID).numItems); }
inline size_t Mesh::numIndices() const  { return size_t(renderer()->indexAllocation(_indexDataID).numItems); }
inline size_t Mesh::numPrimitiveSets() const  { return size_t(_attribStorage->renderer()->primitiveSetAllocation(_primitiveSetDataID).numItems); }

inline const ArrayAllocation<Mesh>& Mesh::attribAllocation() const  { return _attribStorage->attribAllocation(_attribDataID); }
inline ArrayAllocation<Mesh>& Mesh::attribAllocation()  { return _attribStorage->attribAllocation(_attribDataID); }
inline const ArrayAllocation<Mesh>& Mesh::indexAllocation() const  { return renderer()->indexAllocation(_indexDataID); }
inline ArrayAllocation<Mesh>& Mesh::indexAllocation()  { return renderer()->indexAllocation(_indexDataID); }
inline const ArrayAllocation<Mesh>& Mesh::primitiveSetAllocation() const  { return renderer()->primitiveSetAllocation(_primitiveSetDataID); }
inline ArrayAllocation<Mesh>& Mesh::primitiveSetAllocation()  { return renderer()->primitiveSetAllocation(_primitiveSetDataID); }

inline void Mesh::uploadAttribs(const std::vector<std::vector<uint8_t>>& vertexData,size_t dstIndex)  { _attribStorage->uploadAttribs(*this,vertexData,dstIndex); }
inline void Mesh::uploadAttrib(uint32_t attribIndex,const std::vector<uint8_t>& attribData,size_t dstIndex)  { _attribStorage->uploadAttrib(*this,attribIndex,attribData,dstIndex); }
inline StagingBuffer Mesh::createStagingBuffer(uint32_t attribIndex)  { return _attribStorage->createStagingBuffer(*this,attribIndex); }
inline StagingBuffer Mesh::createStagingBuffer(uint32_t attribIndex,size_t dstIndex,size_t numItems)  { return _attribStorage->createStagingBuffer(*this,attribIndex,dstIndex,numItems); }
inline std::vector<StagingBuffer> Mesh::createStagingBuffers()  { return _attribStorage->createStagingBuffers(*this); }
inline std::vector<StagingBuffer> Mesh::createStagingBuffers(size_t dstIndex,size_t numItems)  { return _attribStorage->createStagingBuffers(*this,dstIndex,numItems); }

inline void Mesh::uploadIndices(std::vector<uint32_t>&& indexData,size_t dstIndex)  { renderer()->uploadIndices(*this,std::move(indexData),dstIndex); }
inline StagingBuffer Mesh::createIndexStagingBuffer()  { return renderer()->createIndexStagingBuffer(*this); }
inline StagingBuffer Mesh::createIndexStagingBuffer(size_t firstIndex,size_t numIndices)  { return renderer()->createIndexStagingBuffer(*this,firstIndex,numIndices); }

inline void Mesh::uploadPrimitiveSets(std::vector<PrimitiveSetGpuData>&& primitiveSetData,size_t dstPrimitiveSet)  { renderer()->uploadPrimitiveSets(*this,std::move(primitiveSetData),dstPrimitiveSet); }
inline StagingBuffer Mesh::createPrimitiveSetStagingBuffer()  { return renderer()->createPrimitiveSetStagingBuffer(*this); }
inline StagingBuffer Mesh::createPrimitiveSetStagingBuffer(size_t firstPrimitiveSet,size_t numPrimitiveSets)  { return renderer()->createPrimitiveSetStagingBuffer(*this,firstPrimitiveSet,numPrimitiveSets); }

}
