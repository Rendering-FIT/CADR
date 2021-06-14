#pragma once

#include <CadR/AllocationManagers.h>
#include <CadR/AttribSizeList.h>
#include <CadR/StagingManager.h>
#include <vulkan/vulkan.hpp>

namespace CadR {

class Geometry;
class Renderer;


/** \brief VertexStorage class provides GPU storage for vertex attributes
 *  of the scene geometry. All the data are stored in vk::Buffers.
 *  Any number of Geometry objects can be stored in a single VertexStorage
 *  as long as all the stored Geometries have the same attribute configuration (see AttribSizeList)
 *  and as long as there is the free space in the buffers.
 */
class CADR_EXPORT VertexStorage final {
protected:

	ArrayAllocationManager _allocationManager;  ///< Allocation manager for attribute data.
	StagingManagerList _stagingManagerList;  ///< Staging managers for the vertex data updates using StagingBuffers.
	AttribSizeList _attribSizeList;  ///< List of the attribute sizes of the single vertex. All vertices stored in this VertexStorage use the same attribute sizes.
	Renderer* _renderer;  ///< Rendering context associated with the VertexStorage.

	std::vector<vk::Buffer> _bufferList;
	std::vector<vk::DeviceMemory> _memoryList;

public:

	VertexStorage(const AttribSizeList& attribSizeList,size_t initialNumVertices=131072);
	VertexStorage(Renderer* r,const AttribSizeList& attribSizeList,size_t initialNumVertices=131072);
	~VertexStorage();

	VertexStorage() = delete;
	VertexStorage(const VertexStorage&) = delete;
	VertexStorage& operator=(const VertexStorage&) = delete;
	VertexStorage(VertexStorage&&) = delete;
	VertexStorage& operator=(VertexStorage&&) = delete;

	void alloc(Geometry& g,size_t numVertices);
	void realloc(Geometry& g,size_t numVertices);
	void free(Geometry& g);

	void upload(Geometry& g,const std::vector<std::vector<uint8_t>>& vertexData);
	void upload(Geometry& g,uint32_t attribIndex,const std::vector<uint8_t>& attribData);
	void uploadSubset(Geometry& g,const std::vector<std::vector<uint8_t>>& vertexData,size_t firstVertex);
	void uploadSubset(Geometry& g,uint32_t attribIndex,const std::vector<uint8_t>& attribData,size_t firstVertex);
	StagingBuffer& createStagingBuffer(Geometry& g,uint32_t attribIndex);
	StagingBuffer& createSubsetStagingBuffer(Geometry& g,uint32_t attribIndex,size_t firstVertex,size_t numVertices);
	std::vector<StagingBuffer*> createStagingBuffers(Geometry& g);
	std::vector<StagingBuffer*> createSubsetStagingBuffers(Geometry& g,size_t firstVertex,size_t numVertices);
	StagingManager* stagingManager(Geometry& g);
	const StagingManager* stagingManager(const Geometry& g) const;

	const ArrayAllocation& allocation(const Geometry& g) const; ///< Returns the vertex allocation for the Geometry.
	ArrayAllocation& allocation(Geometry& g);  ///< Returns the attribute allocation for the Geometry. Modify the returned data only with caution.
	const ArrayAllocationManager& allocationManager() const;  ///< Returns the allocation manager.
	ArrayAllocationManager& allocationManager();  ///< Returns the allocation manager.

	StagingManagerList& stagingManagerList();
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
inline const ArrayAllocation& VertexStorage::allocation(const Geometry& g) const  { return _allocationManager[g.vertexDataID()]; }
inline ArrayAllocation& VertexStorage::allocation(Geometry& g)  { return _allocationManager[g.vertexDataID()]; }
inline const ArrayAllocationManager& VertexStorage::allocationManager() const  { return _allocationManager; }
inline ArrayAllocationManager& VertexStorage::allocationManager()  { return _allocationManager; }
inline StagingManager* VertexStorage::stagingManager(Geometry& g)  { return _allocationManager[g.vertexDataID()].stagingManager; }
inline const StagingManager* VertexStorage::stagingManager(const Geometry& g) const  { return _allocationManager[g.vertexDataID()].stagingManager; }
inline StagingManagerList& VertexStorage::stagingManagerList()  { return _stagingManagerList; }
inline const AttribSizeList& VertexStorage::attribSizeList() const  { return _attribSizeList; }
inline Renderer* VertexStorage::renderer() const  { return _renderer; }
inline const std::vector<vk::Buffer>& VertexStorage::bufferList() const  { return _bufferList; }
inline const std::vector<vk::DeviceMemory>& VertexStorage::memoryList() const  { return _memoryList; }
inline vk::Buffer VertexStorage::buffer(uint32_t attribIndex) const  { return _bufferList[attribIndex]; }
inline vk::DeviceMemory VertexStorage::memory(uint32_t attribIndex) const  { return _memoryList[attribIndex]; }
inline uint32_t VertexStorage::numAttribs() const  { return uint32_t(_attribSizeList.size()); }

}
