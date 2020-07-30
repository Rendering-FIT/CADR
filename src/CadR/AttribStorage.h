#pragma once

#include <CadR/AllocationManagers.h>
#include <CadR/AttribSizeList.h>
#include <vulkan/vulkan.hpp>

namespace CadR {

class Geometry;
class Renderer;
class StagingBuffer;


/** \brief AttribStorage class provides GPU storage for vertex attributes
 *  of the scene geometry. All the data are stored in vk::Buffers.
 *  Any number of Geometry objects can be stored in a single AttribStorage
 *  as long as all the stored Geometries have the same attribute configuration (see AttribSizeList)
 *  and as long as there is the free space in the buffers.
 */
class CADR_EXPORT AttribStorage final {
protected:

	ArrayAllocationManager<Geometry> _allocationManager;  ///< Allocation manager for attribute data.
	AttribSizeList _attribSizeList;  ///< List of the attribute sizes of the single vertex. All vertices stored in this AttribStorage use the same attribute sizes.
	Renderer* _renderer;  ///< Rendering context associated with the AttribStorage.

	std::vector<vk::Buffer> _bufferList;
	std::vector<vk::DeviceMemory> _memoryList;

public:

	AttribStorage() = delete;
	AttribStorage(const AttribSizeList& attribSizeList);
	AttribStorage(Renderer* r,const AttribSizeList& attribSizeList);
	~AttribStorage();

	AttribStorage(const AttribStorage&) = delete;
	AttribStorage(AttribStorage&&) = default;
	AttribStorage& operator=(const AttribStorage&) = delete;
	AttribStorage& operator=(AttribStorage&&) = default;

	void allocAttribs(Geometry& g,size_t num);
	void reallocAttribs(Geometry& g,size_t num);
	void freeAttribs(Geometry& g);

	void uploadAttribs(Geometry& g,const std::vector<std::vector<uint8_t>>& vertexData,size_t firstVertex=0);
	void uploadAttrib(Geometry& g,uint32_t attribIndex,const std::vector<uint8_t>& attribData,size_t firstVertex=0);
	StagingBuffer createStagingBuffer(Geometry& g,uint32_t attribIndex);
	StagingBuffer createStagingBuffer(Geometry& g,uint32_t attribIndex,size_t firstVertex,size_t numVertices);
	std::vector<StagingBuffer> createStagingBuffers(Geometry& g);
	std::vector<StagingBuffer> createStagingBuffers(Geometry& g,size_t firstVertex,size_t numVertices);

	const ArrayAllocation<Geometry>& attribAllocation(uint32_t id) const; ///< Returns the attribute allocation for particular id.
	ArrayAllocation<Geometry>& attribAllocation(uint32_t id);  ///< Returns the attribute allocation for particular id. Modify the returned data only with caution.
	const ArrayAllocationManager<Geometry>& allocationManager() const;  ///< Returns the allocation manager.
	ArrayAllocationManager<Geometry>& allocationManager();  ///< Returns the allocation manager.

	const AttribSizeList& attribSizeList() const;
	Renderer* renderer() const;

	const std::vector<vk::Buffer>& bufferList() const;
	const std::vector<vk::DeviceMemory>& memoryList() const;
	vk::Buffer buffer(uint32_t index) const;
	vk::DeviceMemory memory(uint32_t index) const;
	uint32_t numBuffers() const;

	void render();
	void cancelAllAllocations();

};


}



// inline and template methods
#include <CadR/Geometry.h>
namespace CadR {

inline void AttribStorage::allocAttribs(Geometry& g,size_t num)  { g.allocAttribs(num); }
inline void AttribStorage::reallocAttribs(Geometry& g,size_t num)  { g.reallocAttribs(num); }
inline void AttribStorage::freeAttribs(Geometry& g)  { g.freeAttribs(); }
inline const ArrayAllocation<Geometry>& AttribStorage::attribAllocation(uint32_t id) const  { return _allocationManager[id]; }
inline ArrayAllocation<Geometry>& AttribStorage::attribAllocation(uint32_t id)  { return _allocationManager[id]; }
inline const ArrayAllocationManager<Geometry>& AttribStorage::allocationManager() const  { return _allocationManager; }
inline ArrayAllocationManager<Geometry>& AttribStorage::allocationManager()  { return _allocationManager; }
inline const AttribSizeList& AttribStorage::attribSizeList() const  { return _attribSizeList; }
inline Renderer* AttribStorage::renderer() const  { return _renderer; }
inline const std::vector<vk::Buffer>& AttribStorage::bufferList() const  { return _bufferList; }
inline const std::vector<vk::DeviceMemory>& AttribStorage::memoryList() const  { return _memoryList; }
inline vk::Buffer AttribStorage::buffer(uint32_t index) const  { return _bufferList[index]; }
inline vk::DeviceMemory AttribStorage::memory(uint32_t index) const  { return _memoryList[index]; }
inline uint32_t AttribStorage::numBuffers() const  { return uint32_t(_bufferList.size()); }

}
