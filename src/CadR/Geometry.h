#pragma once

#include <CadR/DataAllocation.h>
#include <CadR/Drawable.h>

namespace CadR {

class Renderer;


/** Geometry class represents data used for rendering.
 *  It is composed of vertex data (vertex attributes), index data and primitiveSets.
 *
 *  CADR is designed to handle very large number of Geometry objects rendered in real-time.
 *  Hundreds of thousands geometries should be reachable on nowadays hi-end systems.
 *  This is important for many CAD applications because CAD scenes are often composed
 *  of very high number of parts of various detail.
 *
 *  Drawable class is used to render Geometry using particular StateSet
 *  and instanced by transformation list.
 */
class CADR_EXPORT Geometry {
protected:

	DataAllocation _vertices;       ///< Memory allocation of vertices in GPU memory.
	DataAllocation _indices;        ///< Memory allocation of indices in GPU memory.
	DataAllocation _primitiveSets;  ///< Memory allocation of PrimitiveSets in GPU memory. It is used to construct draw commands.
	DrawableList _drawableList;     ///< List of all Drawables referencing this Geometry.

	friend Drawable;

public:

	// construction and destruction
	Geometry(Renderer& r);  ///< Constructs the object without allocating or reserving any memory.
	Geometry(const Geometry&) = delete;  ///< No copy constructor.
	Geometry(Geometry&& other) = default;  ///< Move constructor.
	~Geometry();  ///< Destructor.

	// operators
	Geometry& operator=(const Geometry&) = delete;  ///< No copy assignment.
	Geometry& operator=(Geometry&& rhs) = default;  ///< Move assignment operator.

	// getters
	Renderer& renderer() const;
	DataStorage& dataStorage() const;
	size_t vertexDataSize() const;
	size_t indexDataSize() const;
	size_t primitiveSetDataSize() const;

	// allocation structures
	DataAllocation& vertexDataAllocation();  ///< Returns the vertex data allocation. Modify the returned data only with caution.
	const DataAllocation& vertexDataAllocation() const; ///< Returns the vertex data allocation.
	DataAllocation& indexDataAllocation();   ///< Returns the index data allocation. Modify the returned data only with caution.
	const DataAllocation& indexDataAllocation() const;  ///< Returns the index data allocation.
	DataAllocation& primitiveSetDataAllocation();  ///< Returns the primitiveSet data allocation. Modify the returned data only with caution.
	const DataAllocation& primitiveSetDataAllocation() const;  ///< Returns the primitiveSet data allocation.

	// data upload
	void uploadVertexData(const void* ptr, size_t numBytes);
	void uploadIndexData (const void* ptr, size_t numBytes);
	void uploadPrimitiveSetData(const void* ptr, size_t numBytes);
	StagingData createVertexStagingData(size_t numBytes);
	StagingData createIndexStagingData(size_t numBytes);
	StagingData createPrimitiveSetStagingData(size_t numBytes);

	// release memory
	void freeVertexData();
	void freeIndexData();
	void freePrimitiveSetData();
	void freeData();

};


}


// inline methods
#include <CadR/DataAllocation.h>
#include <CadR/DataStorage.h>
#include <CadR/Renderer.h>
namespace CadR {

inline Geometry::Geometry(Renderer& r) : _vertices(r.dataStorage()), _indices(r.dataStorage()), _primitiveSets(r.dataStorage())  {}
inline Geometry::~Geometry()  {}

inline Renderer& Geometry::renderer() const  { return _vertices.dataStorage().renderer(); }
inline DataStorage& Geometry::dataStorage() const  { return _vertices.dataStorage(); }
inline size_t Geometry::vertexDataSize() const  { return _vertices.size(); }
inline size_t Geometry::indexDataSize() const  { return _indices.size(); }
inline size_t Geometry::primitiveSetDataSize() const  { return _primitiveSets.size(); }

inline DataAllocation& Geometry::vertexDataAllocation()  { return _vertices; }
inline const DataAllocation& Geometry::vertexDataAllocation() const  { return _vertices; }
inline DataAllocation& Geometry::indexDataAllocation()  { return _indices; }
inline const DataAllocation& Geometry::indexDataAllocation() const  { return _indices; }
inline DataAllocation& Geometry::primitiveSetDataAllocation()  { return _primitiveSets; }
inline const DataAllocation& Geometry::primitiveSetDataAllocation() const  { return _primitiveSets; }

inline void Geometry::uploadVertexData(const void* ptr, size_t numBytes)  { StagingData sd=_vertices.alloc(numBytes); memcpy(sd.data(), ptr, numBytes); }
inline void Geometry::uploadIndexData (const void* ptr, size_t numBytes)  { StagingData sd=_indices.alloc(numBytes);  memcpy(sd.data(), ptr, numBytes); }
inline void Geometry::uploadPrimitiveSetData(const void* ptr, size_t numBytes)  { StagingData sd=_primitiveSets.alloc(numBytes); memcpy(sd.data(), ptr, numBytes); }
inline StagingData Geometry::createVertexStagingData(size_t numBytes)  { return _vertices.alloc(numBytes); }
inline StagingData Geometry::createIndexStagingData(size_t numBytes)  { return _indices.alloc(numBytes); }
inline StagingData Geometry::createPrimitiveSetStagingData(size_t numBytes)  { return _primitiveSets.alloc(numBytes); }

inline void Geometry::freeVertexData()  { _vertices.free(); }
inline void Geometry::freeIndexData()  { _indices.free(); }
inline void Geometry::freePrimitiveSetData()  { _primitiveSets.free(); }
inline void Geometry::freeData()  { freeVertexData(); freeIndexData(); freePrimitiveSetData(); }

}
