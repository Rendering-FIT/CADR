#pragma once

#include <CadR/AllocationManagers.h>
#include <CadR/AttribSizeList.h>
#include <CadR/StagingManager.h>
#include <vulkan/vulkan.hpp>

namespace CadR {

class Renderer;
class GeometryMemory;


/** \brief GeometryStorage class provides GPU data storage for Geometry objects.
 *  It stores vertex attributes, index data and PrimitiveSet data.
 *  All the data are stored in vk::Buffers.
 *  Any number of Geometry objects can be stored in single GeometryStorage
 *  as long as all the stored Geometries have the same attribute configuration (see AttribSizeList)
 *  and as long as there is the free space in the buffers or as long as more buffers can be allocated.
 *
 *  Because in CAD applications, the amount of required memory for GeometryStorage
 *  often cannot be determined in advance, and reallocation of large memory blocks
 *  might be a performance problem, the memory is managed in smaller memory blocks of fixed size
 *  encapsulated in GeometryMemory class. Each GeometryStorage can use any number of
 *  GeometryMemory objects to provide for particular needs of each application.
 * 
 *  \sa GeometryMemory.
 */
class CADR_EXPORT GeometryStorage {
protected:

	Renderer* _renderer;  ///< Rendering context associated with the GeometryStorage.
	std::vector<std::unique_ptr<GeometryMemory>> _geometryMemoryList;
	AttribSizeList _attribSizeList;  ///< List of the attribute sizes of single vertex. All vertices stored in this GeometryStorage use the same attribute sizes.
	uint32_t _highestGeometryMemoryId = ~0u;
	std::vector<uint32_t> _freeGeometryMemoryIds;

public:

	GeometryStorage(const AttribSizeList& attribSizeList, size_t initialVertexCapacity = 0);
	GeometryStorage(Renderer* r, const AttribSizeList& attribSizeList, size_t initialVertexCapacity = 0);
	GeometryStorage(const AttribSizeList& attribSizeList, size_t initialVertexCapacity, size_t initialIndexCapacity, size_t initialPrimitiveSetCapacity);
	GeometryStorage(Renderer* r, const AttribSizeList& attribSizeList, size_t initialVertexCapacity, size_t initialIndexCapacity, size_t initialPrimitiveSetCapacity);
	~GeometryStorage();

	GeometryStorage() = delete;
	GeometryStorage(const GeometryStorage&) = delete;
	GeometryStorage& operator=(const GeometryStorage&) = delete;
	GeometryStorage(GeometryStorage&&) = delete;
	GeometryStorage& operator=(GeometryStorage&&) = delete;

	std::vector<std::unique_ptr<GeometryMemory>>& geometryMemoryList();  ///< Returns GeometryMemory list. It is generally better to use standard Geometry's alloc/realloc/free methods than trying to modify the list directly.
	const std::vector<std::unique_ptr<GeometryMemory>>& geometryMemoryList() const;  ///< Returns GeometryMemory list.
	const AttribSizeList& attribSizeList() const;
	size_t numAttribs() const;
	Renderer* renderer() const;

	void render();
	void cancelAllAllocations();

	uint32_t allocGeometryMemoryId() noexcept;
	void freeGeometryMemoryId(uint32_t id) noexcept;

};


}


// inline and template methods
namespace CadR {

inline std::vector<std::unique_ptr<GeometryMemory>>& GeometryStorage::geometryMemoryList()  { return _geometryMemoryList; }
inline const std::vector<std::unique_ptr<GeometryMemory>>& GeometryStorage::geometryMemoryList() const  { return _geometryMemoryList; }
inline const AttribSizeList& GeometryStorage::attribSizeList() const  { return _attribSizeList; }
inline Renderer* GeometryStorage::renderer() const  { return _renderer; }
inline size_t GeometryStorage::numAttribs() const  { return _attribSizeList.size(); }
inline uint32_t GeometryStorage::allocGeometryMemoryId() noexcept  { if(_freeGeometryMemoryIds.empty()) return ++_highestGeometryMemoryId; uint32_t v=_freeGeometryMemoryIds.back(); _freeGeometryMemoryIds.pop_back(); return v; }

}
