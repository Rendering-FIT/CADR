#pragma once

#include <CadR/AllocationManagers.h>
#include <CadR/AttribSizeList.h>
#include <vulkan/vulkan.hpp>

namespace CadR {

class Geometry;
class Renderer;
class StagingBuffer;


/** \brief VertexStorage class provides GPU storage for vertex attributes
 *  of the scene geometry. All the data are stored in vk::Buffers.
 *  Any number of Geometry objects can be stored in a single VertexStorage
 *  as long as all the stored Geometries have the same attribute configuration (see AttribSizeList)
 *  and as long as there is the free space in the buffers.
 */
class CADR_EXPORT VertexStorage final {
protected:

	ArrayAllocationManager<Geometry> _allocationManager;  ///< Allocation manager for attribute data.
	AttribSizeList _attribSizeList;  ///< List of the attribute sizes of the single vertex. All vertices stored in this VertexStorage use the same attribute sizes.
	Renderer* _renderer;  ///< Rendering context associated with the VertexStorage.

	std::vector<vk::Buffer> _bufferList;
	std::vector<vk::DeviceMemory> _memoryList;

public:

	VertexStorage(const AttribSizeList& attribSizeList,size_t initialNumVertices=1024);
	VertexStorage(Renderer* r,const AttribSizeList& attribSizeList,size_t initialNumVertices=1024);
	VertexStorage(VertexStorage&&) = default;
	VertexStorage& operator=(VertexStorage&&) = default;
	~VertexStorage();

	VertexStorage() = delete;
	VertexStorage(const VertexStorage&) = delete;
	VertexStorage& operator=(const VertexStorage&) = delete;

	void alloc(Geometry& g,size_t numVertices);
	void realloc(Geometry& g,size_t numVertices);
	void free(Geometry& g);

	void upload(Geometry& g,const std::vector<std::vector<uint8_t>>& vertexData,size_t firstVertex=0);
	void upload(Geometry& g,uint32_t attribIndex,const std::vector<uint8_t>& attribData,size_t firstVertex=0);
	StagingBuffer createStagingBuffer(Geometry& g,uint32_t attribIndex);
	StagingBuffer createStagingBuffer(Geometry& g,uint32_t attribIndex,size_t firstVertex,size_t numVertices);
	std::vector<StagingBuffer> createStagingBuffers(Geometry& g);
	std::vector<StagingBuffer> createStagingBuffers(Geometry& g,size_t firstVertex,size_t numVertices);

	const ArrayAllocation<Geometry>& allocation(uint32_t id) const; ///< Returns the vertex allocation for particular id.
	ArrayAllocation<Geometry>& allocation(uint32_t id);  ///< Returns the attribute allocation for particular id. Modify the returned data only with caution.
	const ArrayAllocationManager<Geometry>& allocationManager() const;  ///< Returns the allocation manager.
	ArrayAllocationManager<Geometry>& allocationManager();  ///< Returns the allocation manager.

	const AttribSizeList& attribSizeList() const;
	Renderer* renderer() const;

	const std::vector<vk::Buffer>& bufferList() const;
	const std::vector<vk::DeviceMemory>& memoryList() const;
	vk::Buffer buffer(uint32_t attribIndex) const;
	vk::DeviceMemory memory(uint32_t attribIndex) const;
	uint32_t numAttribs() const;
	void reallocStorage(size_t newNumVertices);

	void render();
	void cancelAllAllocations();

};


}



// inline and template methods
#include <CadR/Geometry.h>
namespace CadR {

inline void VertexStorage::alloc(Geometry& g,size_t num)  { g.allocVertices(num); }
inline void VertexStorage::realloc(Geometry& g,size_t num)  { g.reallocVertices(num); }
inline void VertexStorage::free(Geometry& g)  { g.freeVertices(); }
inline const ArrayAllocation<Geometry>& VertexStorage::allocation(uint32_t id) const  { return _allocationManager[id]; }
inline ArrayAllocation<Geometry>& VertexStorage::allocation(uint32_t id)  { return _allocationManager[id]; }
inline const ArrayAllocationManager<Geometry>& VertexStorage::allocationManager() const  { return _allocationManager; }
inline ArrayAllocationManager<Geometry>& VertexStorage::allocationManager()  { return _allocationManager; }
inline const AttribSizeList& VertexStorage::attribSizeList() const  { return _attribSizeList; }
inline Renderer* VertexStorage::renderer() const  { return _renderer; }
inline const std::vector<vk::Buffer>& VertexStorage::bufferList() const  { return _bufferList; }
inline const std::vector<vk::DeviceMemory>& VertexStorage::memoryList() const  { return _memoryList; }
inline vk::Buffer VertexStorage::buffer(uint32_t attribIndex) const  { return _bufferList[attribIndex]; }
inline vk::DeviceMemory VertexStorage::memory(uint32_t attribIndex) const  { return _memoryList[attribIndex]; }
inline uint32_t VertexStorage::numAttribs() const  { return uint32_t(_attribSizeList.size()); }

}
