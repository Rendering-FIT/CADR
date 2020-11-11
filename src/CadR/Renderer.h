#ifndef CADR_RENDERER_H
#define CADR_RENDERER_H

#include <CadR/AllocationManagers.h>
#include <vulkan/vulkan.hpp>
#include <list>
#include <map>
#include <memory>

namespace CadR {

class AttribSizeList;
class Drawable;
class Geometry;
class StagingBuffer;
class StateSet;
class VertexStorage;
class VulkanDevice;
class VulkanInstance;
struct DrawableGpuData;
struct PrimitiveSetGpuData;


class Renderer final {
private:

	VulkanDevice* _device;
	uint32_t _graphicsQueueFamily;
	vk::Queue _graphicsQueue;
	vk::PhysicalDeviceMemoryProperties _memoryProperties;
	vk::DeviceSize nonCoherentAtom_addition;
	vk::DeviceSize nonCoherentAtom_mask;

	std::map<AttribSizeList,std::list<VertexStorage>> _vertexStorages;
	VertexStorage*   _emptyStorage;
	vk::Buffer       _indexBuffer;
	vk::DeviceMemory _indexBufferMemory;
	vk::Buffer       _dataStorageBuffer;
	vk::DeviceMemory _dataStorageMemory;
	vk::Buffer       _primitiveSetBuffer;
	vk::DeviceMemory _primitiveSetBufferMemory;
	vk::Buffer       _drawableBuffer;
	vk::DeviceMemory _drawableBufferMemory;
	size_t           _drawableBufferSize = 0;
	vk::Buffer       _drawableStagingBuffer;
	vk::DeviceMemory _drawableStagingMemory;
	DrawableGpuData* _drawableStagingData;
	vk::Buffer       _matrixListControlBuffer;
	vk::DeviceMemory _matrixListControlBufferMemory;
	vk::Buffer       _drawIndirectBuffer;
	vk::DeviceMemory _drawIndirectMemory;
	ArrayAllocationManager<Geometry> _indexAllocationManager;  ///< Allocation manager for index data.
	ArrayAllocationManager<uint32_t> _dataStorageAllocationManager;  ///< Allocation manager for data storage.
	ArrayAllocationManager<Geometry> _primitiveSetAllocationManager;  ///< Allocation manager for primitiveSet data.

	vk::CommandPool _transientCommandPool;
	vk::CommandBuffer _uploadingCommandBuffer;
	std::list<std::tuple<vk::Buffer,vk::DeviceMemory>> _objectsToDeleteAfterCopyOperation;

	vk::PipelineCache _pipelineCache;
	vk::ShaderModule _drawableShader;
	vk::DescriptorPool _drawableDescriptorPool;
	vk::DescriptorSetLayout _drawableDescriptorSetLayout;
	vk::DescriptorSet _drawableDescriptorSet;
	vk::PipelineLayout _drawablePipelineLayout;
	vk::Pipeline _drawablePipeline;

	static Renderer* _defaultRenderer;

public:

	CADR_EXPORT static Renderer* get();
	CADR_EXPORT static void set(Renderer* r);

	CADR_EXPORT Renderer(bool makeDefault=true);
	CADR_EXPORT Renderer(VulkanDevice& device,VulkanInstance& instance,vk::PhysicalDevice physicalDevice,
	                     uint32_t graphicsQueueFamily,bool makeDefault=true);
	CADR_EXPORT ~Renderer();
	CADR_EXPORT void init(VulkanDevice& device,VulkanInstance& instance,vk::PhysicalDevice physicalDevice,
	                      uint32_t graphicsQueueFamily,bool makeDefault=true);
	CADR_EXPORT void finalize();
	CADR_EXPORT void leakResources();

	CADR_EXPORT size_t prepareSceneRendering(StateSet& stateSetRoot);
	CADR_EXPORT void recordDrawableProcessing(vk::CommandBuffer commandBuffer,size_t numDrawables);
	CADR_EXPORT void recordSceneRendering(vk::CommandBuffer commandBuffer,StateSet& stateSetRoot,vk::RenderPass renderPass,
	                                      vk::Framebuffer framebuffer,const vk::Rect2D& renderArea,
	                                      uint32_t clearValueCount, const vk::ClearValue* clearValues);

	CADR_EXPORT VulkanDevice* device() const;
	CADR_EXPORT uint32_t graphicsQueueFamily() const;
	CADR_EXPORT vk::Queue graphicsQueue() const;
	CADR_EXPORT const vk::PhysicalDeviceMemoryProperties& memoryProperties() const;

	CADR_EXPORT vk::Buffer indexBuffer() const;
	CADR_EXPORT vk::Buffer dataStorageBuffer() const;
	CADR_EXPORT vk::Buffer primitiveSetBuffer() const;
	CADR_EXPORT vk::Buffer drawableBuffer() const;
	CADR_EXPORT size_t drawableBufferSize() const;
	CADR_EXPORT vk::Buffer drawableStagingBuffer() const;
	CADR_EXPORT DrawableGpuData* drawableStagingData() const;
	CADR_EXPORT vk::Buffer matrixListControlBuffer() const;
	CADR_EXPORT vk::Buffer drawIndirectBuffer() const;

	CADR_EXPORT vk::PipelineCache pipelineCache() const;
	CADR_EXPORT vk::DescriptorSet drawableDescriptorSet() const;
	CADR_EXPORT vk::PipelineLayout drawablePipelineLayout() const;
	CADR_EXPORT vk::Pipeline drawablePipeline() const;

	CADR_EXPORT VertexStorage* getOrCreateVertexStorage(const AttribSizeList& attribSizeList);
	CADR_EXPORT std::map<AttribSizeList,std::list<VertexStorage>>& getVertexStorages();
	CADR_EXPORT const VertexStorage* emptyStorage() const;
	CADR_EXPORT VertexStorage* emptyStorage();

	CADR_EXPORT vk::DeviceMemory allocateMemory(vk::Buffer buffer,vk::MemoryPropertyFlags requiredFlags);
	CADR_EXPORT vk::CommandBuffer uploadingCommandBuffer() const;
	CADR_EXPORT void scheduleCopyOperation(StagingBuffer& stagingBuffer);
	CADR_EXPORT void executeCopyOperations();

	CADR_EXPORT const ArrayAllocation<Geometry>& indexAllocation(uint32_t id) const;  ///< Returns index allocation for particular id.
	CADR_EXPORT ArrayAllocation<Geometry>& indexAllocation(uint32_t id);   ///< Returns index allocation for particular id. Modify the returned data only with caution.
	CADR_EXPORT const ArrayAllocationManager<Geometry>& indexAllocationManager() const;
	CADR_EXPORT ArrayAllocationManager<Geometry>& indexAllocationManager();

	CADR_EXPORT void uploadIndices(Geometry& g,std::vector<uint32_t>& indexData,size_t dstIndex=0);
	CADR_EXPORT StagingBuffer createIndexStagingBuffer(Geometry& g);
	CADR_EXPORT StagingBuffer createIndexStagingBuffer(Geometry& g,size_t firstIndex,size_t numIndices);

	CADR_EXPORT const ArrayAllocation<uint32_t>& dataStorageAllocation(uint32_t id) const;  ///< Returns dataStorage allocation for particular id.
	CADR_EXPORT ArrayAllocation<uint32_t>& dataStorageAllocation(uint32_t id);   ///< Returns dataStorage allocation for particular id. Modify the returned data only with caution.
	CADR_EXPORT const ArrayAllocationManager<uint32_t>& dataStorageAllocationManager() const;
	CADR_EXPORT ArrayAllocationManager<uint32_t>& dataStorageAllocationManager();

	CADR_EXPORT void uploadDataStorage(uint32_t id,std::vector<uint8_t>& data,size_t dstIndex=0);
	CADR_EXPORT void uploadDataStorage(uint32_t id,const void* data,size_t size,size_t dstIndex=0);
	CADR_EXPORT StagingBuffer createDataStorageStagingBuffer(uint32_t id);
	CADR_EXPORT StagingBuffer createDataStorageStagingBuffer(uint32_t id,size_t offset,size_t size);

	CADR_EXPORT const ArrayAllocation<Geometry>& primitiveSetAllocation(uint32_t id) const;  ///< Returns primitiveSet allocation for particular id.
	CADR_EXPORT ArrayAllocation<Geometry>& primitiveSetAllocation(uint32_t id);   ///< Returns primitiveSet allocation for particular id. Modify the returned data only with caution.
	CADR_EXPORT const ArrayAllocationManager<Geometry>& primitiveSetAllocationManager() const;
	CADR_EXPORT ArrayAllocationManager<Geometry>& primitiveSetAllocationManager();

	CADR_EXPORT void uploadPrimitiveSets(Geometry& g,std::vector<PrimitiveSetGpuData>& primitiveSetData,size_t dstPrimitiveSetIndex=0);
	CADR_EXPORT StagingBuffer createPrimitiveSetStagingBuffer(Geometry& g);
	CADR_EXPORT StagingBuffer createPrimitiveSetStagingBuffer(Geometry& g,size_t firstPrimitiveSet,size_t numPrimitiveSets);

protected:
	CADR_EXPORT void purgeObjectsToDeleteAfterCopyOperation();
};


// inline methods
inline Renderer* Renderer::get()  { return _defaultRenderer; }
inline void Renderer::set(Renderer* r)  { _defaultRenderer=r; }
inline VulkanDevice* Renderer::device() const  { return _device; }
inline uint32_t Renderer::graphicsQueueFamily() const  { return _graphicsQueueFamily; }
inline vk::Queue Renderer::graphicsQueue() const  { return _graphicsQueue; }
inline const vk::PhysicalDeviceMemoryProperties& Renderer::memoryProperties() const  { return _memoryProperties; }
inline vk::Buffer Renderer::indexBuffer() const  { return _indexBuffer; }
inline vk::Buffer Renderer::dataStorageBuffer() const  { return _dataStorageBuffer; }
inline vk::Buffer Renderer::primitiveSetBuffer() const  { return _primitiveSetBuffer; }
inline vk::Buffer Renderer::drawableBuffer() const  { return _drawableBuffer; }
inline size_t Renderer::drawableBufferSize() const  { return _drawableBufferSize; }
inline vk::Buffer Renderer::drawableStagingBuffer() const  { return _drawableStagingBuffer; }
inline DrawableGpuData* Renderer::drawableStagingData() const  { return _drawableStagingData; }
inline vk::Buffer Renderer::matrixListControlBuffer() const  { return _matrixListControlBuffer; }
inline vk::Buffer Renderer::drawIndirectBuffer() const  { return _drawIndirectBuffer; }
inline vk::PipelineCache Renderer::pipelineCache() const  { return _pipelineCache; }
inline vk::DescriptorSet Renderer::drawableDescriptorSet() const  { return _drawableDescriptorSet; }
inline vk::PipelineLayout Renderer::drawablePipelineLayout() const  { return _drawablePipelineLayout; }
inline vk::Pipeline Renderer::drawablePipeline() const  { return _drawablePipeline; }
inline std::map<AttribSizeList,std::list<VertexStorage>>& Renderer::getVertexStorages()  { return _vertexStorages; }
inline const VertexStorage* Renderer::emptyStorage() const  { return _emptyStorage; }
inline VertexStorage* Renderer::emptyStorage()  { return _emptyStorage; }
inline const ArrayAllocation<Geometry>& Renderer::indexAllocation(uint32_t id) const  { return _indexAllocationManager[id]; }
inline ArrayAllocation<Geometry>& Renderer::indexAllocation(uint32_t id)  { return _indexAllocationManager[id]; }
inline const ArrayAllocationManager<Geometry>& Renderer::indexAllocationManager() const  { return _indexAllocationManager; }
inline ArrayAllocationManager<Geometry>& Renderer::indexAllocationManager()  { return _indexAllocationManager; }
inline const ArrayAllocation<uint32_t>& Renderer::dataStorageAllocation(uint32_t id) const  { return _dataStorageAllocationManager[id]; }
inline ArrayAllocation<uint32_t>& Renderer::dataStorageAllocation(uint32_t id)  { return _dataStorageAllocationManager[id]; }
inline const ArrayAllocationManager<uint32_t>& Renderer::dataStorageAllocationManager() const  { return _dataStorageAllocationManager; }
inline ArrayAllocationManager<uint32_t>& Renderer::dataStorageAllocationManager()  { return _dataStorageAllocationManager; }
inline const ArrayAllocation<Geometry>& Renderer::primitiveSetAllocation(uint32_t id) const  { return _primitiveSetAllocationManager[id]; }
inline ArrayAllocation<Geometry>& Renderer::primitiveSetAllocation(uint32_t id)  { return _primitiveSetAllocationManager[id]; }
inline const ArrayAllocationManager<Geometry>& Renderer::primitiveSetAllocationManager() const  { return _primitiveSetAllocationManager; }
inline ArrayAllocationManager<Geometry>& Renderer::primitiveSetAllocationManager()  { return _primitiveSetAllocationManager; }
inline vk::CommandBuffer Renderer::uploadingCommandBuffer() const  { return _uploadingCommandBuffer; }

}


#endif /* CADR_RENDERER_H */
