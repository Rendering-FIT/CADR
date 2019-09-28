#pragma once

#include <vulkan/vulkan.hpp>
#include <CadR/Export.h>
#include <CadR/AttribSizeList.h>
#include <CadR/AllocationManagers.h>

namespace CadR {

class Mesh;
class Renderer;
class StagingBuffer;


/** \brief AttribStorage class provides GPU storage for vertex attributes
 *  of the scene geometry. All the data are stored in vk::Buffers.
 *  Any number of Mesh objects can be stored in a single AttribStorage
 *  as long as all the stored Meshes have the same attribute configuration (see AttribConfig)
 *  and as long as there is the free space in the buffers.
 */
class CADR_EXPORT AttribStorage final {
protected:

	ArrayAllocationManager<Mesh> _allocationManager;  ///< Allocation manager for attribute data.
	AttribSizeList _attribSizeList;  ///< List of the attribute sizes of the single vertex. All vertices stored in this AttribStorage use the same attribute sizes.
	Renderer* _renderer;  ///< Rendering context associated with the AttribStorage.

	std::vector<vk::Buffer> _bufferList;
	std::vector<vk::DeviceMemory> _memoryList;

public:

	AttribStorage() = delete;
	AttribStorage(Renderer* r,const AttribSizeList& attribSizeList);
	~AttribStorage();

	AttribStorage(const AttribStorage&) = delete;
	AttribStorage(AttribStorage&&) = default;
	AttribStorage& operator=(const AttribStorage&) = delete;
	AttribStorage& operator=(AttribStorage&&) = default;

	void allocAttribs(Mesh& m,size_t num);
	void reallocAttribs(Mesh& m,size_t num);
	void freeAttribs(Mesh& m);

	void uploadAttribs(Mesh& m,const std::vector<std::vector<uint8_t>>& vertexData,size_t firstVertex=0);
	void uploadAttrib(Mesh& m,unsigned attribIndex,const std::vector<uint8_t>& attribData,size_t firstVertex=0);
	StagingBuffer createStagingBuffer(Mesh& m,unsigned attribIndex);
	StagingBuffer createStagingBuffer(Mesh& m,unsigned attribIndex,size_t firstVertex,size_t numVertices);
	std::vector<StagingBuffer> createStagingBuffers(Mesh& m);
	std::vector<StagingBuffer> createStagingBuffers(Mesh& m,size_t firstVertex,size_t numVertices);

	const ArrayAllocation<Mesh>& attribAllocation(unsigned id) const; ///< Returns the attribute allocation for particular id.
	ArrayAllocation<Mesh>& attribAllocation(unsigned id);  ///< Returns the attribute allocation for particular id. Modify the returned data only with caution.
	const ArrayAllocationManager<Mesh>& allocationManager() const;  ///< Returns the allocation manager.
	ArrayAllocationManager<Mesh>& allocationManager();  ///< Returns the allocation manager.

	const AttribSizeList& attribSizeList() const;
	Renderer* renderer() const;

	const std::vector<vk::Buffer>& bufferList() const;
	const std::vector<vk::DeviceMemory>& memoryList() const;
	vk::Buffer buffer(unsigned index) const;
	vk::DeviceMemory memory(unsigned index) const;
	unsigned numBuffers() const;

	void render();
	void cancelAllAllocations();

};


}



// inline and template methods
#include <CadR/Mesh.h>
namespace CadR {

inline void AttribStorage::allocAttribs(Mesh& m,size_t num)  { m.allocAttribs(num); }
inline void AttribStorage::reallocAttribs(Mesh& m,size_t num)  { m.reallocAttribs(num); }
inline void AttribStorage::freeAttribs(Mesh& m)  { m.freeAttribs(); }
inline const ArrayAllocation<Mesh>& AttribStorage::attribAllocation(unsigned id) const  { return _allocationManager[id]; }
inline ArrayAllocation<Mesh>& AttribStorage::attribAllocation(unsigned id)  { return _allocationManager[id]; }
inline const ArrayAllocationManager<Mesh>& AttribStorage::allocationManager() const  { return _allocationManager; }
inline ArrayAllocationManager<Mesh>& AttribStorage::allocationManager()  { return _allocationManager; }
inline const AttribSizeList& AttribStorage::attribSizeList() const  { return _attribSizeList; }
inline Renderer* AttribStorage::renderer() const  { return _renderer; }
inline const std::vector<vk::Buffer>& AttribStorage::bufferList() const  { return _bufferList; }
inline const std::vector<vk::DeviceMemory>& AttribStorage::memoryList() const  { return _memoryList; }
inline vk::Buffer AttribStorage::buffer(unsigned index) const  { return _bufferList[index]; }
inline vk::DeviceMemory AttribStorage::memory(unsigned index) const  { return _memoryList[index]; }
inline unsigned AttribStorage::numBuffers() const  { return unsigned(_bufferList.size()); }

}
