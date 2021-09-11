#include <CadR/GeometryMemory.h>
#include <CadR/GeometryStorage.h>
#include <CadR/Geometry.h>
#include <CadR/PrimitiveSet.h>
#include <CadR/Renderer.h>
#include <CadR/VulkanDevice.h>
#include <numeric>

using namespace std;
using namespace CadR;


GeometryMemory::GeometryMemory(GeometryStorage* geometryStorage, size_t vertexCapacity, size_t indexCapacity, size_t primitiveSetCapacity)
	: _geometryStorage(geometryStorage)
	, _vertexAllocationManager(uint32_t(vertexCapacity), 0)  // set capacity to vertexCapacity, set null object (on index 0) to be of zero size
	, _indexAllocationManager(uint32_t(indexCapacity), 0)  // set capacity to indexCapacity, set null object (on index 0) to be of zero size
	, _primitiveSetAllocationManager(uint32_t(primitiveSetCapacity), 0)  // set capacity to primitiveSetCapacity, set null object (on index 0) to be of zero size
	, _id(geometryStorage->allocGeometryMemoryId())
{
	// compute offsets and buffer size
	const AttribSizeList& attribSizeList = geometryStorage->attribSizeList();
	_attribOffsets.reserve(attribSizeList.size());
	Renderer* r = geometryStorage->renderer();
	size_t offset = 0;
	auto it = attribSizeList.begin();
	for(auto e=attribSizeList.end(); it!=e; it++) {
		_attribOffsets.push_back(offset);
		offset = r->alignStandardBuffer(offset + (*it)*vertexCapacity);
	}
	_indexOffset = offset;
	offset = r->alignStandardBuffer(offset + sizeof(uint32_t)*indexCapacity);
	_primitiveSetOffset = offset;
	_size = offset + sizeof(PrimitiveSetGpuData)*primitiveSetCapacity;

	// handle zero-sized buffer
	if(_size == 0)
		return;

	// create _buffer
	VulkanDevice* device=r->device();
	_buffer = device->createBuffer(
		vk::BufferCreateInfo(
			vk::BufferCreateFlags(),  // flags
			_size,  // size
			vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer |  // usage
				vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress |
				vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
			vk::SharingMode::eExclusive,  // sharingMode
			0,  // queueFamilyIndexCount
			nullptr  // pQueueFamilyIndices
		)
	);

	// allocate _memory
	_memory = r->allocatePointerAccessMemory(_buffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

	// bind memory
	device->bindBufferMemory(
		_buffer,  // buffer
		_memory,  // memory
		0   // memoryOffset
	);

	// get buffer address
	_bufferDeviceAddress = 
		device->getBufferDeviceAddress(
			vk::BufferDeviceAddressInfo(
				_buffer  // buffer
			)
		);
}


GeometryMemory::~GeometryMemory()
{
	// destroy staging stuff
	_vertexStagingManagerList.clear_and_dispose([](StagingManager* sm){delete sm;});
	_indexStagingManagerList.clear_and_dispose([](StagingManager* sm){delete sm;});
	_primitiveSetStagingManagerList.clear_and_dispose([](StagingManager* sm){delete sm;});

	// free id
	_geometryStorage->freeGeometryMemoryId(_id);

	// destroy buffers
	VulkanDevice* device = _geometryStorage->renderer()->device();
	device->destroy(_buffer);

	// free memory
	device->freeMemory(_memory);
}


bool GeometryMemory::tryAlloc(size_t numVertices, size_t numIndices, size_t numPrimitiveSets,
                              uint32_t& vertexDataID, uint32_t& indexDataID, uint32_t& primitiveSetDataID)
{
	if(!_vertexAllocationManager.canAlloc(numVertices) ||
	   !_indexAllocationManager.canAlloc(numIndices) ||
	   !_primitiveSetAllocationManager.canAlloc(numPrimitiveSets))
		return false;

	vertexDataID = _vertexAllocationManager.alloc(numVertices, this);
	indexDataID = _indexAllocationManager.alloc(numIndices, this);
	primitiveSetDataID = _primitiveSetAllocationManager.alloc(numPrimitiveSets, this);
	return true;
}


bool GeometryMemory::tryRealloc(size_t numVertices, size_t numIndices, size_t numPrimitiveSets,
                                uint32_t& vertexDataID, uint32_t& indexDataID, uint32_t& primitiveSetDataID)
{
	const ArrayAllocation& va = _vertexAllocationManager.operator[](vertexDataID);
	const ArrayAllocation& ia = _indexAllocationManager.operator[](indexDataID);
	const ArrayAllocation& pa = _primitiveSetAllocationManager.operator[](primitiveSetDataID);
	if(numVertices <= va.numItems) {
		if(numIndices <= ia.numItems) {
			if(numPrimitiveSets <= pa.numItems) {

				// shrink allocations
				if(numVertices != va.numItems)
					_vertexAllocationManager.shrink(vertexDataID, numVertices);
				if(numIndices != ia.numItems)
					_indexAllocationManager.shrink(indexDataID, numIndices);
				if(numPrimitiveSets != pa.numItems)
					_primitiveSetAllocationManager.shrink(primitiveSetDataID, numPrimitiveSets);
				return true;

			}
			else
				if(!_primitiveSetAllocationManager.canRealloc(primitiveSetDataID, numPrimitiveSets))
					return false;
		}
		else {
			if(!_indexAllocationManager.canRealloc(indexDataID, numIndices) ||
			   !_primitiveSetAllocationManager.canRealloc(primitiveSetDataID, numPrimitiveSets))
				return false;
		}
	}
	else {
		if(!_vertexAllocationManager.canRealloc(vertexDataID, numVertices) ||
		   !_indexAllocationManager.canRealloc(indexDataID, numIndices) ||
		   !_primitiveSetAllocationManager.canRealloc(primitiveSetDataID, numPrimitiveSets))
			return false;
	}

	// realloc
	_vertexAllocationManager.realloc(vertexDataID, numVertices);
	_indexAllocationManager.realloc(indexDataID, numIndices);
	_primitiveSetAllocationManager.realloc(primitiveSetDataID, numPrimitiveSets);
	return true;
}


void GeometryMemory::free(uint32_t vertexDataID, uint32_t indexDataID, uint32_t primitiveSetDataID)
{
	_vertexAllocationManager.free(vertexDataID);
	_indexAllocationManager.free(indexDataID);
	_primitiveSetAllocationManager.free(primitiveSetDataID);
}


void GeometryMemory::cancelAllAllocations()
{
	// break all Geometry references to this GeometryMemory
	for(auto it=_vertexAllocationManager.begin(); it!=_vertexAllocationManager.end(); it++)
		if(it->owner)  reinterpret_cast<Geometry*>(it->owner)->_vertexDataID = 0;
	for(auto it=_indexAllocationManager.begin(); it!=_indexAllocationManager.end(); it++)
		if(it->owner)  reinterpret_cast<Geometry*>(it->owner)->_indexDataID = 0;
	for(auto it=_primitiveSetAllocationManager.begin(); it!=_primitiveSetAllocationManager.end(); it++)
		if(it->owner)  reinterpret_cast<Geometry*>(it->owner)->_primitiveSetDataID = 0;

	// empty allocation maps
	_vertexAllocationManager.clear();
	_indexAllocationManager.clear();
	_primitiveSetAllocationManager.clear();
}
