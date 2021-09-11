#ifndef CADR_RENDERER_H
#define CADR_RENDERER_H

#include <CadR/AllocationManagers.h>
#include <CadR/FrameInfo.h>
#include <CadR/StagingManager.h>
#include <vulkan/vulkan.hpp>
#include <list>
#include <map>

namespace CadR {

class AttribSizeList;
class Geometry;
class GeometryMemory;
class GeometryStorage;
class StateSet;
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
	vk::DeviceSize _standardBufferAlignment;  ///< Memory alignment of a standard buffer. It is used for optimization purposes like putting more small buffers into one large buffer.
	vk::DeviceSize nonCoherentAtom_addition;  ///< Serves for memory alignment purposes. It is equivalent to PhysicalDeviceLimits::nonCoherentAtomSize-1.
	vk::DeviceSize nonCoherentAtom_mask;  ///< Serves for memory alignment purposes. It is equivalent to bitwise negation of nonCoherentAtom_addition value.

	std::map<AttribSizeList, GeometryStorage> _geometryStorageMap;
	GeometryStorage* _emptyStorage;
	GeometryMemory*  _emptyGeometryMemory;
	vk::Buffer       _dataStorageBuffer;
	vk::DeviceMemory _dataStorageMemory;
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
	ArrayAllocationManager _dataStorageAllocationManager;  ///< Allocation manager for data storage.

	StagingManagerList _dataStorageStagingManagerList;
	vk::CommandPool _transientCommandPool;
	vk::CommandBuffer _uploadingCommandBuffer;
	vk::CommandPool _precompiledCommandPool;
	vk::CommandBuffer _readTimestampCommandBuffer;
	vk::QueryPool _readTimestampQueryPool;
	vk::Fence _fence;  ///< Fence for general synchronization.

	vk::PipelineCache _pipelineCache;
	vk::ShaderModule _drawableShader;
	vk::DescriptorPool _drawableDescriptorPool;
	vk::DescriptorSetLayout _drawableDescriptorSetLayout;
	vk::DescriptorSet _drawableDescriptorSet;
	vk::PipelineLayout _drawablePipelineLayout;
	vk::Pipeline _drawablePipeline;

	size_t _frameNumber = ~size_t(0);  ///< Monotonically increasing frame number. The first frame is 0. The initial value is -1, marking pre-first frame time.
	double _cpuTimestampPeriod;  ///< The time period of cpu timestamp begin incremented by 1. The period is given in seconds.
	float _gpuTimestampPeriod;  ///< The time period of gpu timestamp being incremented by 1. The period is given in seconds.
	bool _collectFrameInfo = false;  ///< True if frame timing information collecting is enabled.
	bool _useCalibratedTimestamps;  ///< True if use of calibrated timestamps is enabled. It requires VK_EXT_calibrated_timestamps Vulkan extension to be present and enabled.
	vk::TimeDomainEXT _timestampHostTimeDomain;  ///< Time domain used for the cpu timestamps.
	struct FrameInfoStuff {
		FrameInfo info;  ///< FrameInfo data.
		vk::UniqueHandle<vk::QueryPool,VulkanDevice> timestampPool;  ///< QueryPool for timestamps that are collected during frame rendering.
	};
	std::list<FrameInfoStuff> _inProgressFrameInfoList;  ///< List of FrameInfo structures whose data are still being collected.
	std::list<FrameInfo> _completedFrameInfoList;  ///< List of FrameInfo structures whose data are complete and ready to be handed to the user.
	uint32_t _timestampIndex;  ///< Timestamp index used during recording of frame to track the index of the next timestamp that will be recorded to the command buffer.

	static Renderer* _defaultRenderer;

public:

	CADR_EXPORT static Renderer* get();
	CADR_EXPORT static void set(Renderer* r);
	CADR_EXPORT static size_t initialDataStorageCapacity;

	CADR_EXPORT Renderer(bool makeDefault=true);
	CADR_EXPORT Renderer(VulkanDevice& device,VulkanInstance& instance,vk::PhysicalDevice physicalDevice,
	                     uint32_t graphicsQueueFamily,bool makeDefault=true);
	CADR_EXPORT ~Renderer();
	CADR_EXPORT void init(VulkanDevice& device,VulkanInstance& instance,vk::PhysicalDevice physicalDevice,
	                      uint32_t graphicsQueueFamily,bool makeDefault=true);
	CADR_EXPORT void finalize();
	CADR_EXPORT void leakResources();

	CADR_EXPORT size_t beginFrame();  ///< Call this method to mark the beginning of the frame rendering. It returns the frame number assigned to this frame. It is the same number as frameNumber() will return from now on until the next call to beginFrame().
	CADR_EXPORT void beginRecording(vk::CommandBuffer commandBuffer);  ///< Start recording of the command buffer.
	CADR_EXPORT size_t prepareSceneRendering(StateSet& stateSetRoot);
	CADR_EXPORT void recordDrawableProcessing(vk::CommandBuffer commandBuffer,size_t numDrawables);
	CADR_EXPORT void recordSceneRendering(vk::CommandBuffer commandBuffer,StateSet& stateSetRoot,vk::RenderPass renderPass,
	                                      vk::Framebuffer framebuffer,const vk::Rect2D& renderArea,
	                                      uint32_t clearValueCount, const vk::ClearValue* clearValues);
	CADR_EXPORT void endRecording(vk::CommandBuffer commandBuffer);  ///< Finish recording of the command buffer.
	CADR_EXPORT void endFrame();  ///< Mark the end of frame recording. This is usually called after the command buffer is submitted to gpu for execution.

	CADR_EXPORT VulkanDevice* device() const;
	CADR_EXPORT uint32_t graphicsQueueFamily() const;
	CADR_EXPORT vk::Queue graphicsQueue() const;
	CADR_EXPORT const vk::PhysicalDeviceMemoryProperties& memoryProperties() const;
	CADR_EXPORT size_t standardBufferAlignment() const;
	CADR_EXPORT size_t alignStandardBuffer(size_t offset) const;
	CADR_EXPORT size_t getVertexCapacityForBuffer(const AttribSizeList& attribSizeList,size_t bufferSize) const;

	CADR_EXPORT size_t frameNumber() const;  ///< Returns the frame number of the Renderer. The first frame number is zero and increments for each rendered frame. The initial value is -1 until the first frame rendering starts. The frame number is incremented in beginFrame() and the same value is returned until the next call to beginFrame().
	CADR_EXPORT bool collectFrameInfo() const;  ///< Returns whether frame rendering information is collected.
	CADR_EXPORT void setCollectFrameInfo(bool on, bool useCalibratedTimestamps = false);  ///< Sets whether collecting of frame rendering information will be performed. The method should not be called between beginFrame() and endFrame().
	CADR_EXPORT void setCollectFrameInfo(bool on, bool useCalibratedTimestamps, vk::TimeDomainEXT timestampHostTimeDomain);  ///< Sets whether collecting of frame rendering information will be performed. The method should not be called between beginFrame() and endFrame().
	CADR_EXPORT std::list<FrameInfo> getFrameInfos();  ///< Returns list of FrameInfo for the frames whose collecting of information already completed. Some info is collected as gpu progresses on the frame rendering. The method returns info only about those frames whose collecting of information already finished. To avoid unnecessary memory consumption, unretrieved FrameInfos are deleted after some time. To avoid lost FrameInfos, it is enough to call getFrameInfos() once between each endFrame() and beginFrame() of two consecutive frames.
	CADR_EXPORT double cpuTimestampPeriod() const;  ///< The time period of cpu timestamp begin incremented by 1. The period is given in seconds.
	CADR_EXPORT float gpuTimestampPeriod() const;  ///< The time period of gpu timestamp being incremented by 1. The period is given in seconds.
	CADR_EXPORT uint64_t getCpuTimestamp() const;  ///< Returns the cpu timestamp.
	CADR_EXPORT uint64_t getGpuTimestamp() const;  ///< Returns the gpu timestamp.

	CADR_EXPORT vk::Buffer dataStorageBuffer() const;
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

	CADR_EXPORT GeometryStorage* getOrCreateGeometryStorage(const AttribSizeList& attribSizeList);
	CADR_EXPORT std::map<AttribSizeList,GeometryStorage>& geometryStorageMap();
	CADR_EXPORT GeometryStorage* emptyStorage();
	CADR_EXPORT const GeometryStorage* emptyStorage() const;
	CADR_EXPORT GeometryMemory* emptyGeometryMemory();
	CADR_EXPORT const GeometryMemory* emptyGeometryMemory() const;

	CADR_EXPORT vk::DeviceMemory allocateMemory(vk::Buffer buffer,vk::MemoryPropertyFlags requiredFlags);
	CADR_EXPORT vk::DeviceMemory allocatePointerAccessMemory(vk::Buffer buffer,vk::MemoryPropertyFlags requiredFlags);
	CADR_EXPORT void executeCopyOperations();

	CADR_EXPORT const ArrayAllocation& dataStorageAllocation(uint32_t id) const;  ///< Returns dataStorage allocation for particular id.
	CADR_EXPORT ArrayAllocation& dataStorageAllocation(uint32_t id);   ///< Returns dataStorage allocation for particular id. Modify the returned data only with caution.
	CADR_EXPORT const ArrayAllocationManager& dataStorageAllocationManager() const;
	CADR_EXPORT ArrayAllocationManager& dataStorageAllocationManager();

	CADR_EXPORT void uploadDataStorage(uint32_t id,std::vector<uint8_t>& data);
	CADR_EXPORT void uploadDataStorage(uint32_t id,const void* data,size_t size);
	CADR_EXPORT void uploadDataStorageSubset(uint32_t id,std::vector<uint8_t>& data,size_t offset);
	CADR_EXPORT void uploadDataStorageSubset(uint32_t id,const void* data,size_t size,size_t offset);
	CADR_EXPORT StagingBuffer& createDataStorageStagingBuffer(uint32_t id);
	CADR_EXPORT StagingBuffer& createDataStorageSubsetStagingBuffer(uint32_t id,size_t offset,size_t size);

	CADR_EXPORT StagingManagerList& dataStorageStagingManagerList();

};


}

// inline methods
namespace CadR {
inline Renderer* Renderer::get()  { return _defaultRenderer; }
inline void Renderer::set(Renderer* r)  { _defaultRenderer=r; }
inline VulkanDevice* Renderer::device() const  { return _device; }
inline uint32_t Renderer::graphicsQueueFamily() const  { return _graphicsQueueFamily; }
inline vk::Queue Renderer::graphicsQueue() const  { return _graphicsQueue; }
inline const vk::PhysicalDeviceMemoryProperties& Renderer::memoryProperties() const  { return _memoryProperties; }
inline size_t Renderer::standardBufferAlignment() const  { return _standardBufferAlignment; }
inline size_t Renderer::alignStandardBuffer(size_t offset) const  { size_t a=_standardBufferAlignment-1; return (offset+a)&(~a); }
inline size_t Renderer::frameNumber() const  { return _frameNumber; }
inline bool Renderer::collectFrameInfo() const  { return _collectFrameInfo; }
inline double Renderer::cpuTimestampPeriod() const  { return _cpuTimestampPeriod; }
inline float Renderer::gpuTimestampPeriod() const  { return _gpuTimestampPeriod; }
inline vk::Buffer Renderer::dataStorageBuffer() const  { return _dataStorageBuffer; }
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
inline std::map<AttribSizeList, GeometryStorage>& Renderer::geometryStorageMap()  { return _geometryStorageMap; }
inline GeometryStorage* Renderer::emptyStorage()  { return _emptyStorage; }
inline const GeometryStorage* Renderer::emptyStorage() const  { return _emptyStorage; }
inline GeometryMemory* Renderer::emptyGeometryMemory()  { return _emptyGeometryMemory; }
inline const GeometryMemory* Renderer::emptyGeometryMemory() const  { return _emptyGeometryMemory; }
inline const ArrayAllocation& Renderer::dataStorageAllocation(uint32_t id) const  { return _dataStorageAllocationManager[id]; }
inline ArrayAllocation& Renderer::dataStorageAllocation(uint32_t id)  { return _dataStorageAllocationManager[id]; }
inline const ArrayAllocationManager& Renderer::dataStorageAllocationManager() const  { return _dataStorageAllocationManager; }
inline ArrayAllocationManager& Renderer::dataStorageAllocationManager()  { return _dataStorageAllocationManager; }
inline void Renderer::uploadDataStorage(uint32_t id,std::vector<uint8_t>& data)  { uploadDataStorage(id,data.data(),data.size()); }
inline void Renderer::uploadDataStorageSubset(uint32_t id,std::vector<uint8_t>& data,size_t offset)  { uploadDataStorageSubset(id,data.data(),data.size(),offset); }
inline StagingManagerList& Renderer::dataStorageStagingManagerList()  { return _dataStorageStagingManagerList; }

}

#endif /* CADR_RENDERER_H */
