#pragma once

#include <memory>
#include <vector>
#include <CADR/Export.h>
#include <CADR/DrawCommand.h>

namespace cd {

class AttribConfig;
struct Buffer;


/** Drawable class represents geometry data that can be rendered.
 *  It is composed of vertex data (vertex attributes), indices (optionally) and draw commands.
 *
 *  The class is meant to handle hundreds of thousands rendered drawables in real-time.
 *  This is useful for many CAD applications and scenes composed of many objects.
 *
 *  Geode class is used to render Drawables using particular StateSet and instancing them by
 *  evaluating Transformation graph.
 */
class CADR_EXPORT Drawable {
protected:

	AttribStorage* _attribStorage;  ///< AttribStorage where vertex and index data are stored.
	size_t _verticesDataId;  ///< Id of vertex data allocation inside AttribStorage.
	size_t _indicesDataId;  ///< Id of index data allocation inside AttribStorage.
	size_t _drawCommandDataId;  ///< Id od DrawCommand data allocation.
	DrawCommandList _drawCommandList;

public:

	static inline Drawable* create(Renderer* r);
	static inline std::shared_ptr<Drawable> make_shared(Renderer* r);
	inline Drawable();  ///< Default constructor. Object is largerly uninitialized, valid() returns false and attribStorage() nullptr. Call init() before using the object.
	inline Drawable(Renderer *r,const AttribConfig& ac,size_t numVertices,
	                size_t numIndices,size_t numDrawCommands);  ///< Constructs the object by parameters.
	inline Drawable(Drawable&& d);  ///< Move constructor.
	inline Drawable& operator=(Drawable&& rhs);  ///< Move operator.
	virtual ~Drawable()  { freeData(); }  ///< Destructor.
	inline void init(Renderer *r,const AttribConfig& ac,size_t numVertices,
	                 size_t numIndices,size_t numDrawCommands);

	Drawable(const Drawable&) = delete;  ///< No copy constructor. Object copies are not allowed. Only moves.
	Drawable& operator=(const Drawable&) = delete;  ///< No assignment operator. Only move operator is allowed.

	//?inline void set(AttribStorage* a,size_t vId,size_t iId,size_t dcId);

	inline AttribStorage* attribStorage() const;
	inline size_t verticesDataId() const;
	inline size_t indicesDataId() const;
	inline size_t drawCommandDataId() const;
	inline bool valid() const;

	inline const DrawCommandList& drawCommandList() const;
	//?inline DrawCommandList& drawCommandList();

	inline void allocData(const AttribConfig& ac,size_t numVertices,
	                      size_t numIndices,size_t numDrawCommands);
	inline void reallocData(size_t numVertices,size_t numIndices,
	                        size_t numDrawCommands,bool preserveContent=true);
	inline void freeData();

	inline size_t numVertices() const;
	inline size_t numIndices() const;
	inline size_t numPrimitives() const;

	inline void uploadVertices(std::vector<Buffer>&& vertexData,size_t dstIndex=0);
	inline void uploadAttrib(unsigned attribIndex,Buffer&& attribData,size_t dstIndex=0);
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

	friend AttribStorage;
};


}


// inline methods
#include <CADR/Factory.h>
#include <CADR/Renderer.h>
namespace cd {

inline Drawable* Drawable::create(Renderer* r)  { return Factory::get()->createDrawable(r); }
inline std::shared_ptr<Drawable> Drawable::make_shared(Renderer* r)  { return Factory::get()->makeDrawable(r); }
inline Drawable::Drawable() : _attribStorage(nullptr)  {}
inline Drawable::Drawable(Drawable&& d) : _attribStorage(d._attribStorage), _verticesDataId(d._verticesDataId), _indicesDataId(d._indicesDataId), _drawCommandDataId(d._drawCommandDataId), _drawCommandList(std::move(d._drawCommandList))  { d._attribStorage=nullptr; }
inline Drawable& Drawable::operator=(Drawable&& d)  { _attribStorage=d._attribStorage; _verticesDataId=d._verticesDataId; _indicesDataId=d._indicesDataId; _drawCommandDataId=d._drawCommandDataId; _drawCommandList=std::move(d._drawCommandList); d._attribStorage=nullptr; return *this; }
inline Drawable::Drawable(Renderer* r,const AttribConfig& ac,size_t numVertices,size_t numIndices,size_t numDrawCommands) : _attribStorage(nullptr)  { r->allocDrawableData(this,ac,numVertices,numIndices,numDrawCommands); }
inline void Drawable::init(Renderer *r,const AttribConfig& ac,size_t numVertices,size_t numIndices,size_t numDrawCommands)  { freeData(); r->allocDrawableData(this,ac,numVertices,numIndices,numDrawCommands); }
inline AttribStorage* Drawable::attribStorage() const  { return _attribStorage; }
inline size_t Drawable::verticesDataId() const  { return _verticesDataId; }
inline size_t Drawable::indicesDataId() const  { return _indicesDataId; }
inline size_t Drawable::drawCommandDataId() const  { return _drawCommandDataId; }
inline bool Drawable::valid() const  { return _attribStorage!=nullptr; }
inline const DrawCommandList& Drawable::drawCommandList() const  { return _drawCommandList; }
//inline void Drawable::allocData(const AttribConfig& ac,size_t numVertices,size_t numIndices,size_t numDrawCommands)  { _attribStorage->renderer()->
//inline void Drawable::reallocData(size_t numVertices,size_t numIndices,
//								size_t numDrawCommands,bool preserveContent=true);
inline void Drawable::freeData()  { if(_attribStorage) _attribStorage->freeData(this); }
inline size_t Drawable::numVertices() const  { return _attribStorage ? _attribStorage->vertexArrayAllocation(_verticesDataId).numItems : 0; }
inline size_t Drawable::numIndices() const  { return _attribStorage ? _attribStorage->indexArrayAllocation(_indicesDataId).numItems : 0; }
//inline size_t Drawable::numPrimitives() const  { return _attribStorage ? _attribStorage->renderer()->primitiveStorage()->operator[](_primitivesDataId).numItems : 0; }
/*inline void Drawable::uploadVertices(std::vector<Buffer>&& vertexData,size_t dstIndex=0);
inline void Drawable::uploadAttrib(unsigned attribIndex,Buffer&& attribData,size_t dstIndex=0);
inline void Drawable::uploadIndices(std::vector<uint32_t>&& indexData,size_t dstIndex=0);
inline void Drawable::setNullAttribStorage()  { _attribStorage=_attribStorage->nullAttribStorage(); }*/

}
