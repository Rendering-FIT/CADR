#pragma once

#include <CadR/AllocationManagers.h>
#include <CadR/StagingManager.h>
#include <vulkan/vulkan.hpp>

namespace CadR {

class AttribSizeList;
class Geometry;
class GeometryStorage;
struct PrimitiveSetGpuData;


/** \brief GeometryMemory class provides GPU memory functionality
 *  for storing vertex attributes, indices and PrimitiveSet data.
 *  All the data are stored in one or more vk::Buffers.
 *  The buffer(s) are allocated during the object construction
 *  and cannot change their size afterwards. If you need more
 *  space, allocate another GeometryMemory object. Optionally,
 *  you might copy the content of the old GeometryMemory object
 *  into new one and delete the old object. For high-level
 *  geometry memory management, see GeometryStorage.
 */
class CADR_EXPORT GeometryMemory {
protected:

	GeometryStorage* _geometryStorage;  ///< GeometryStorage owning this GeometryMemory.
	ArrayAllocationManager _vertexAllocationManager;  ///< Allocation manager for attribute data.
	ArrayAllocationManager _indexAllocationManager;  ///< Allocation manager for index data.
	ArrayAllocationManager _primitiveSetAllocationManager;  ///< Allocation manager for PrimitiveSet data.
	StagingManagerList _vertexStagingManagerList;  ///< Staging managers for the vertex data updates.
	StagingManagerList _indexStagingManagerList;  ///< Staging managers for the index data updates.
	StagingManagerList _primitiveSetStagingManagerList;  ///< Staging managers for the PrimitiveSet data updates.

	size_t _size;
	vk::Buffer _buffer;
	vk::DeviceMemory _memory;
	vk::DeviceAddress _bufferDeviceAddress;
	std::vector<size_t> _attribOffsets;
	size_t _indexOffset;
	size_t _primitiveSetOffset;
	uint32_t _id;

public:

	// construction and destruction
	GeometryMemory(GeometryStorage* geometryStorage, size_t vertexCapacity);
	GeometryMemory(GeometryStorage* geometryStorage, size_t vertexCapacity, size_t indexCapacity, size_t primitiveSetCapacity);
	~GeometryMemory();

	// deleted constructors and operators
	GeometryMemory() = delete;
	GeometryMemory(const GeometryMemory&) = delete;
	GeometryMemory& operator=(const GeometryMemory&) = delete;
	GeometryMemory(GeometryMemory&&) = delete;
	GeometryMemory& operator=(GeometryMemory&&) = delete;

	// getters
	GeometryStorage* geometryStorage() const;
	size_t vertexCapacity() const;
	size_t indexCapacity() const;
	size_t primitiveSetCapacity() const;
	vk::Buffer buffer() const;
	vk::DeviceMemory memory() const;
	vk::DeviceAddress bufferDeviceAddress() const;
	size_t size() const;
	size_t numAttribs() const;
	const std::vector<size_t>& attribOffsets() const;
	size_t attribOffset(size_t index) const;
	size_t indexOffset() const;
	size_t primitiveSetOffset() const;
	uint32_t id() const;

	// allocation managers
	ArrayAllocationManager& vertexAllocationManager();  ///< Returns the vertex allocation manager.
	const ArrayAllocationManager& vertexAllocationManager() const;  ///< Returns the vertex allocation manager.
	ArrayAllocationManager& indexAllocationManager();  ///< Returns the index allocation manager.
	const ArrayAllocationManager& indexAllocationManager() const;  ///< Returns the index allocation manager.
	ArrayAllocationManager& primitiveSetAllocationManager();  ///< Returns the primitiveSet allocation manager.
	const ArrayAllocationManager& primitiveSetAllocationManager() const;  ///< Returns the primitiveSet allocation manager.

	// staging manager lists
	StagingManagerList& vertexStagingManagerList();
	const StagingManagerList& vertexStagingManagerList() const;
	StagingManagerList& indexStagingManagerList();
	const StagingManagerList& indexStagingManagerList() const;
	StagingManagerList& primitiveSetStagingManagerList();
	const StagingManagerList& primitiveSetStagingManagerList() const;

	bool tryAlloc(size_t numVertices, size_t numIndices, size_t numPrimitiveSets,
	              uint32_t& vertexDataID, uint32_t& indexDataID, uint32_t& primitiveSetDataID);
	bool tryRealloc(size_t numVertices, size_t numIndices, size_t numPrimitiveSets,
	                uint32_t& vertexDataID, uint32_t& indexDataID, uint32_t& primitiveSetDataID);
	void free(uint32_t vertexDataID, uint32_t indexDataID, uint32_t primitiveSetDataID);
	void cancelAllAllocations();

};


}


// inline methods
namespace CadR {

inline GeometryMemory::GeometryMemory(GeometryStorage* geometryStorage, size_t vertexCapacity) : GeometryMemory(geometryStorage, vertexCapacity, vertexCapacity*6, vertexCapacity/8)  {}

inline ArrayAllocationManager& GeometryMemory::vertexAllocationManager()  { return _vertexAllocationManager; }
inline const ArrayAllocationManager& GeometryMemory::vertexAllocationManager() const  { return _vertexAllocationManager; }
inline ArrayAllocationManager& GeometryMemory::indexAllocationManager()  { return _indexAllocationManager; }
inline const ArrayAllocationManager& GeometryMemory::indexAllocationManager() const  { return _indexAllocationManager; }
inline ArrayAllocationManager& GeometryMemory::primitiveSetAllocationManager()  { return _primitiveSetAllocationManager; }
inline const ArrayAllocationManager& GeometryMemory::primitiveSetAllocationManager() const  { return _primitiveSetAllocationManager; }

inline StagingManagerList& GeometryMemory::vertexStagingManagerList()  { return _vertexStagingManagerList; }
inline const StagingManagerList& GeometryMemory::vertexStagingManagerList() const  { return _vertexStagingManagerList; }
inline StagingManagerList& GeometryMemory::indexStagingManagerList()  { return _indexStagingManagerList; }
inline const StagingManagerList& GeometryMemory::indexStagingManagerList() const  { return _indexStagingManagerList; }
inline StagingManagerList& GeometryMemory::primitiveSetStagingManagerList()  { return _primitiveSetStagingManagerList; }
inline const StagingManagerList& GeometryMemory::primitiveSetStagingManagerList() const  { return _primitiveSetStagingManagerList; }

inline GeometryStorage* GeometryMemory::geometryStorage() const  { return _geometryStorage; }
inline size_t GeometryMemory::vertexCapacity() const  { return _vertexAllocationManager.capacity(); }
inline size_t GeometryMemory::indexCapacity() const  { return _indexAllocationManager.capacity(); }
inline size_t GeometryMemory::primitiveSetCapacity() const  { return _primitiveSetAllocationManager.capacity(); }
inline vk::Buffer GeometryMemory::buffer() const  { return _buffer; }
inline vk::DeviceMemory GeometryMemory::memory() const  { return _memory; }
inline vk::DeviceAddress GeometryMemory::bufferDeviceAddress() const  { return _bufferDeviceAddress; }
inline size_t GeometryMemory::size() const  { return _size; }
inline const std::vector<size_t>& GeometryMemory::attribOffsets() const  { return _attribOffsets; }
inline size_t GeometryMemory::numAttribs() const  { return _attribOffsets.size(); }
inline size_t GeometryMemory::attribOffset(size_t index) const  { return _attribOffsets[index]; }
inline size_t GeometryMemory::indexOffset() const  { return _indexOffset; }
inline size_t GeometryMemory::primitiveSetOffset() const  { return _primitiveSetOffset; }
inline uint32_t GeometryMemory::id() const  { return _id; }

}
