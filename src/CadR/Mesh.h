#pragma once

#include <vector>
#include <CadR/Export.h>

namespace CadR {

class AttribConfig;
class AttribStorage;
struct BufferData;
class Renderer;


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
	unsigned _attribDataID = 0;  ///< Id of vertex data allocation inside AttribStorage.
	unsigned _indexDataID = 0;  ///< Id of index data allocation inside AttribStorage.
#if 0
	unsigned _primitiveSetDataID = invalidID;  ///< Id od DrawCommand data allocation.
	DrawCommandList _drawCommandList;
#endif

public:

	Mesh();  ///< Default constructor. Object is largely uninitialized, valid() returns false and attribStorage() nullptr. Call init() before using the object.
	Mesh(const AttribConfig& ac,size_t numVertices,
	     size_t numIndices,size_t numDrawCommands);  ///< Constructs the object by parameters.
	Mesh(Renderer* r);  ///< Constructs the object by parameters.
	Mesh(Renderer* r,const AttribConfig& ac,size_t numVertices,
	     size_t numIndices,size_t numDrawCommands);  ///< Constructs the object by parameters.
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

#if 0
	inline unsigned drawCommandDataId() const;
	inline bool valid() const;
	inline const DrawCommandList& drawCommandList() const;
	//?inline DrawCommandList& drawCommandList();
#endif

	void allocData(const AttribConfig& ac,size_t numVertices,
	               size_t numIndices,size_t numDrawCommands);
	void allocData(size_t numVertices,size_t numIndices,size_t numDrawCommands);
	void reallocData(size_t numVertices,size_t numIndices,size_t numDrawCommands);
	void freeData();

	void allocAttribs(const AttribConfig& ac,size_t num);
	void allocAttribs(size_t numVertices);
	void reallocAttribs(size_t numVertices);
	void freeAttribs();

	void allocIndices(size_t num);
	void reallocIndices(size_t num);
	void freeIndices();

#if 0
	void allocPrimitiveSets(size_t num);
	void reallocPrimitiveSets(size_t num);
	void freePrimitiveSets();
#endif

	size_t numVertices() const;
	size_t numIndices() const;
#if 0
	size_t numPrimitives() const;
#endif

	inline void uploadAttribs(std::vector<BufferData>&& vertexData,size_t dstIndex=0);
	inline void uploadAttrib(unsigned attribIndex,BufferData&& attribData,size_t dstIndex=0);
	inline void uploadIndices(std::vector<uint32_t>&& indexData,size_t dstIndex=0);
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
namespace CadR {

inline Mesh::Mesh() : _attribStorage(Renderer::get()->emptyStorage())  {}
inline Mesh::Mesh(const AttribConfig& ac,size_t numVertices,size_t numIndices,size_t numDrawCommands) : _attribStorage(Renderer::get()->emptyStorage())  { allocData(ac,numVertices,numIndices,numDrawCommands); }
inline Mesh::Mesh(Renderer* r) : _attribStorage(r->emptyStorage())  {}
inline Mesh::Mesh(Renderer* r,const AttribConfig& ac,size_t numVertices,size_t numIndices,size_t numDrawCommands) : _attribStorage(r->emptyStorage())  { allocData(ac,numVertices,numIndices,numDrawCommands); }
#if 0
inline Mesh::Mesh(Mesh&& m) : _attribStorage(m._attribStorage), _verticesDataId(m._verticesDataId), _indicesDataId(m._indicesDataId), _drawCommandDataId(m._drawCommandDataId), _drawCommandList(std::move(m._drawCommandList))  { m._attribStorage=nullptr; }
inline Mesh& Mesh::operator=(Mesh&& m)  { _attribStorage=d._attribStorage; _verticesDataId=d._verticesDataId; _indicesDataId=d._indicesDataId; _drawCommandDataId=d._drawCommandDataId; _drawCommandList=std::move(d._drawCommandList); d._attribStorage=nullptr; return *this; }
#endif
inline Mesh::~Mesh()  { freeData(); }

inline AttribStorage* Mesh::attribStorage() const  { return _attribStorage; }
inline Renderer* Mesh::renderer() const  { return _attribStorage->renderer(); }
inline unsigned Mesh::attribDataID() const  { return _attribDataID; }
inline unsigned Mesh::indexDataID() const  { return _indexDataID; }

#if 0
inline void Drawable::init(Renderer *r,const AttribConfig& ac,size_t numVertices,size_t numIndices,size_t numDrawCommands)  { freeData(); r->allocDrawableData(this,ac,numVertices,numIndices,numDrawCommands); }
inline unsigned Drawable::drawCommandDataId() const  { return _drawCommandDataId; }
inline bool Drawable::valid() const  { return _attribStorage!=nullptr; }
inline const DrawCommandList& Drawable::drawCommandList() const  { return _drawCommandList; }
#endif

inline void Mesh::allocData(const AttribConfig& ac,size_t numVertices,size_t numIndices,size_t /*numPrimitiveSets*/)  { allocAttribs(ac,numVertices); allocIndices(numIndices); }
inline void Mesh::allocData(size_t numVertices,size_t numIndices,size_t /*numPrimitiveSets*/)  { allocAttribs(numVertices); allocIndices(numIndices); }
inline void Mesh::reallocData(size_t numVertices,size_t numIndices,size_t /*numPrimitiveSets*/)  { reallocAttribs(numVertices); reallocIndices(numIndices); }
inline void Mesh::freeData()  { freeAttribs(); freeIndices(); }

inline void Mesh::allocAttribs(const AttribConfig& ac,size_t num)  { freeAttribs(); _attribStorage=renderer()->getOrCreateAttribStorage(ac); _attribDataID=_attribStorage->allocationManager().alloc(num,this); }
inline void Mesh::allocAttribs(size_t num)  { freeAttribs(); _attribDataID=_attribStorage->allocationManager().alloc(num,this); }
inline void Mesh::reallocAttribs(size_t /*num*/)  { /* FIXME: not implemented yet */ }
inline void Mesh::freeAttribs()  { if(_attribDataID==0) return; _attribStorage->allocationManager().free(_attribDataID); _attribDataID=0; }

inline void Mesh::allocIndices(size_t num)  { freeIndices(); _indexDataID=renderer()->indexAllocationManager().alloc(num,this); }
inline void Mesh::reallocIndices(size_t /*num*/)  { /* FIXME: not implemented yet */ }
inline void Mesh::freeIndices()  { if(_indexDataID==0) return; renderer()->indexAllocationManager().free(_indexDataID); _indexDataID=0; }

#if 0
inline void Mesh::allocPrimitiveSets(size_t num)  { freePrimitiveSets(); _primitiveSetDataID=renderer()->primitiveSetAllocationManager().alloc(); }
inline void Mesh::reallocPrimitiveSets(size_t /*num*/)  { /* FIXME: not implemented yet */ }
inline void Mesh::freePrimitiveSets()  { if(_primitiveSetDataID==0) return; renderer()->primitiveSetAllocationManager().free(_primitiveSetDataID); _primitiveSetDataID=0; }
#endif

inline size_t Mesh::numVertices() const  { return size_t(_attribStorage->attribAllocation(_attribDataID).numItems); }
inline size_t Mesh::numIndices() const  { return size_t(renderer()->indexAllocation(_indexDataID).numItems); }
#if 0
inline size_t Drawable::numPrimitives() const  { return _attribStorage ? _attribStorage->renderer()->primitiveStorage()->operator[](_primitivesDataId).numItems : 0; }
inline void Drawable::uploadVertices(std::vector<Buffer>&& vertexData,size_t dstIndex=0);
inline void Drawable::uploadAttrib(unsigned attribIndex,Buffer&& attribData,size_t dstIndex=0);
inline void Drawable::uploadIndices(std::vector<uint32_t>&& indexData,size_t dstIndex=0);
inline void Drawable::setNullAttribStorage()  { _attribStorage=_attribStorage->nullAttribStorage(); }
#endif

}