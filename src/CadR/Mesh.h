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
	unsigned _attribDataID = 0;  ///< ID of vertex data allocation inside AttribStorage.
	unsigned _indexDataID = 0;  ///< ID of index data allocation inside AttribStorage.
	unsigned _primitiveSetDataID = 0;  ///< ID od DrawCommand data allocation.
	DrawableList _drawableList;

public:

	Mesh();  ///< Default constructor. Object is largely uninitialized, valid() returns false and attribStorage() nullptr. Call init() before using the object.
	Mesh(const AttribSizeList& attribSizeList,size_t numVertices,
	     size_t numIndices,size_t numPrimitiveSets);  ///< Constructs the object by parameters.
	Mesh(Renderer* r);  ///< Constructs the object by parameters.
	Mesh(Renderer* r,const AttribSizeList& attribSizeList,size_t numVertices,
	     size_t numIndices,size_t numPrimitiveSets);  ///< Constructs the object by parameters.
	Mesh(const Mesh&) = delete;  ///< No copy constructor. Only move contructors are allowed.
	Mesh(Mesh&& m);  ///< Move constructor.
	Mesh& operator=(const Mesh&) = delete;  ///< No assignment operator. Only move assignment is allowed.
	Mesh& operator=(Mesh&& rhs);  ///< Move operator.
	~Mesh();  ///< Destructor.

#if 0
	inline void init(Renderer *r,const AttribConfig& ac,size_t numVertices,
	                 size_t numIndices,size_t numDrawCommands);
	//?inline void set(AttribStorage* a,size_t vId,size_t iId,size_t dcId);
#endif

	AttribStorage* attribStorage() const;
	Renderer* renderer() const;
	unsigned attribDataID() const;
	unsigned indexDataID() const;
	unsigned primitiveSetDataID() const;

#if 0
	inline unsigned drawCommandDataId() const;
	inline bool valid() const;
	inline const DrawCommandList& drawCommandList() const;
	//?inline DrawCommandList& drawCommandList();
#endif

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

	void uploadAttribs(const std::vector<std::vector<uint8_t>>& vertexData,size_t dstIndex=0);
	void uploadAttrib(unsigned attribIndex,const std::vector<uint8_t>& attribData,size_t dstIndex=0);
	StagingBuffer createStagingBuffer(unsigned attribIndex);
	StagingBuffer createStagingBuffer(unsigned attribIndex,size_t dstIndex,size_t numItems);
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
                                      unsigned numPrimitives,unsigned dstIndex=0);
         inline void setPrimitives(const Primitive *primitiveList,
                                   unsigned numPrimitives,unsigned startIndex=0,
                                   bool truncate=true);
         inline void setAndUploadPrimitives(PrimitiveGpuData *nonConstBufferData,
                                            const Primitive *primitiveList,unsigned numPrimitives);
         inline void setAndUploadPrimitives(PrimitiveGpuData *nonConstBufferData,
                                            const unsigned *modesAndOffsets4,unsigned numPrimitives);
         inline void updateVertexOffsets(void *primitiveBuffer,
                                         const Primitive *primitiveList,unsigned numPrimitives);
         inline static std::vector<Primitive> generatePrimitiveList(
                                         const unsigned *modesAndOffsets4,unsigned numPrimitives);

         inline void clearPrimitives();
         inline void setNumPrimitives(unsigned num);

	inline DrawableId createGeode(MatrixList *matrixList,StateSet *stateSet);
	inline DrawableId createGeode(const unsigned *primitiveIndices,
	                              const unsigned primitiveCount,
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
#if 0
inline Mesh::Mesh(Mesh&& m) : _attribStorage(m._attribStorage), _verticesDataId(m._verticesDataId), _indicesDataId(m._indicesDataId), _drawCommandDataId(m._drawCommandDataId), _drawCommandList(std::move(m._drawCommandList))  { m._attribStorage=nullptr; }
inline Mesh& Mesh::operator=(Mesh&& m)  { _attribStorage=d._attribStorage; _verticesDataId=d._verticesDataId; _indicesDataId=d._indicesDataId; _drawCommandDataId=d._drawCommandDataId; _drawCommandList=std::move(d._drawCommandList); d._attribStorage=nullptr; return *this; }
#endif
inline Mesh::~Mesh()  { freeData(); }

inline AttribStorage* Mesh::attribStorage() const  { return _attribStorage; }
inline Renderer* Mesh::renderer() const  { return _attribStorage->renderer(); }
inline unsigned Mesh::attribDataID() const  { return _attribDataID; }
inline unsigned Mesh::indexDataID() const  { return _indexDataID; }
inline unsigned Mesh::primitiveSetDataID() const  { return _primitiveSetDataID; }

#if 0
inline void Drawable::init(Renderer *r,const AttribConfig& ac,size_t numVertices,size_t numIndices,size_t numDrawCommands)  { freeData(); r->allocDrawableData(this,ac,numVertices,numIndices,numDrawCommands); }
inline unsigned Drawable::drawCommandDataId() const  { return _drawCommandDataId; }
inline bool Drawable::valid() const  { return _attribStorage!=nullptr; }
inline const DrawCommandList& Drawable::drawCommandList() const  { return _drawCommandList; }
#endif

inline void Mesh::allocData(const AttribSizeList& attribSizeList,size_t numVertices,size_t numIndices,size_t numPrimitiveSets)  { allocAttribs(attribSizeList,numVertices); allocIndices(numIndices); allocPrimitiveSets(numPrimitiveSets); }
inline void Mesh::allocData(size_t numVertices,size_t numIndices,size_t numPrimitiveSets)  { allocAttribs(numVertices); allocIndices(numIndices); allocPrimitiveSets(numPrimitiveSets); }
inline void Mesh::reallocData(size_t numVertices,size_t numIndices,size_t numPrimitiveSets)  { reallocAttribs(numVertices); reallocIndices(numIndices); reallocPrimitiveSets(numPrimitiveSets); }
inline void Mesh::freeData()  { freeAttribs(); freeIndices(); }

inline void Mesh::allocAttribs(const AttribSizeList& attribSizeList,size_t num)  { freeAttribs(); _attribStorage=renderer()->getOrCreateAttribStorage(attribSizeList); _attribDataID=_attribStorage->allocationManager().alloc(unsigned(num),this); if(_attribDataID==0) throw std::runtime_error("Mesh: Failed to allocate attribute data."); }
inline void Mesh::allocAttribs(size_t num)  { freeAttribs(); _attribDataID=_attribStorage->allocationManager().alloc(unsigned(num),this); }
inline void Mesh::reallocAttribs(size_t /*num*/)  { /* FIXME: not implemented yet */ }
inline void Mesh::freeAttribs()  { if(_attribDataID==0) return; _attribStorage->allocationManager().free(_attribDataID); _attribDataID=0; }

inline void Mesh::allocIndices(size_t num)  { freeIndices(); _indexDataID=renderer()->indexAllocationManager().alloc(unsigned(num),this); if(_indexDataID==0) throw std::runtime_error("Mesh: Failed to allocate index data."); }
inline void Mesh::reallocIndices(size_t /*num*/)  { /* FIXME: not implemented yet */ }
inline void Mesh::freeIndices()  { if(_indexDataID==0) return; renderer()->indexAllocationManager().free(_indexDataID); _indexDataID=0; }

inline void Mesh::allocPrimitiveSets(size_t num)  { freePrimitiveSets(); _primitiveSetDataID=renderer()->primitiveSetAllocationManager().alloc(unsigned(num),this); if(_primitiveSetDataID==0) throw std::runtime_error("Mesh: Failed to allocate primitiveSet data."); }
inline void Mesh::reallocPrimitiveSets(size_t /*num*/)  { /* FIXME: not implemented yet */ }
inline void Mesh::freePrimitiveSets()  { if(_primitiveSetDataID==0) return; renderer()->primitiveSetAllocationManager().free(_primitiveSetDataID); _primitiveSetDataID=0; }

inline size_t Mesh::numVertices() const  { return size_t(_attribStorage->attribAllocation(_attribDataID).numItems); }
inline size_t Mesh::numIndices() const  { return size_t(renderer()->indexAllocation(_indexDataID).numItems); }
inline size_t Mesh::numPrimitiveSets() const  { return size_t(_attribStorage->renderer()->primitiveSetAllocation(_primitiveSetDataID).numItems); }

inline void Mesh::uploadAttribs(const std::vector<std::vector<uint8_t>>& vertexData,size_t dstIndex)  { _attribStorage->uploadAttribs(*this,vertexData,dstIndex); }
inline void Mesh::uploadAttrib(unsigned attribIndex,const std::vector<uint8_t>& attribData,size_t dstIndex)  { _attribStorage->uploadAttrib(*this,attribIndex,attribData,dstIndex); }
inline StagingBuffer Mesh::createStagingBuffer(unsigned attribIndex)  { return _attribStorage->createStagingBuffer(*this,attribIndex); }
inline StagingBuffer Mesh::createStagingBuffer(unsigned attribIndex,size_t dstIndex,size_t numItems)  { return _attribStorage->createStagingBuffer(*this,attribIndex,dstIndex,numItems); }
inline std::vector<StagingBuffer> Mesh::createStagingBuffers()  { return _attribStorage->createStagingBuffers(*this); }
inline std::vector<StagingBuffer> Mesh::createStagingBuffers(size_t dstIndex,size_t numItems)  { return _attribStorage->createStagingBuffers(*this,dstIndex,numItems); }

inline void Mesh::uploadIndices(std::vector<uint32_t>&& indexData,size_t dstIndex)  { renderer()->uploadIndices(*this,std::move(indexData),dstIndex); }
inline StagingBuffer Mesh::createIndexStagingBuffer()  { return renderer()->createIndexStagingBuffer(*this); }
inline StagingBuffer Mesh::createIndexStagingBuffer(size_t firstIndex,size_t numIndices)  { return renderer()->createIndexStagingBuffer(*this,firstIndex,numIndices); }

inline void Mesh::uploadPrimitiveSets(std::vector<PrimitiveSetGpuData>&& primitiveSetData,size_t dstPrimitiveSet)  { renderer()->uploadPrimitiveSets(*this,std::move(primitiveSetData),dstPrimitiveSet); }
inline StagingBuffer Mesh::createPrimitiveSetStagingBuffer()  { return renderer()->createPrimitiveSetStagingBuffer(*this); }
inline StagingBuffer Mesh::createPrimitiveSetStagingBuffer(size_t firstPrimitiveSet,size_t numPrimitiveSets)  { return renderer()->createPrimitiveSetStagingBuffer(*this,firstPrimitiveSet,numPrimitiveSets); }


}
