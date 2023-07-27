#ifndef CADR_RENDERER_H
#define CADR_RENDERER_H

#include <CadR/DataStorage.h>
#include <CadR/FrameInfo.h>
#include <vulkan/vulkan.hpp>
#include <array>
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


class CADR_EXPORT Renderer {
protected:

	VulkanDevice* _device;
	uint32_t _graphicsQueueFamily;
	vk::Queue _graphicsQueue;
	vk::PhysicalDeviceMemoryProperties _memoryProperties;
	vk::DeviceSize _standardBufferAlignment;  ///< Memory alignment of a standard buffer. It is used for optimization purposes like putting more small buffers into one large buffer.
	vk::DeviceSize _nonCoherentAtom_addition;  ///< Serves for memory alignment purposes. It is equivalent to PhysicalDeviceLimits::nonCoherentAtomSize-1.
	vk::DeviceSize _nonCoherentAtom_mask;  ///< Serves for memory alignment purposes. It is equivalent to bitwise negation of nonCoherentAtom_addition value.
	std::array<size_t, 3> _bufferSizeList;  ///< First, we allocate small DataMemory and GeometryMemory objects. When they are full, we allocate bigger. This list contains the allocation steps. It is usually something like 64KiB, 2MiB and 32MiB. We try to not allocate too big buffers as it cause performance spikes.

	std::map<AttribSizeList, GeometryStorage> _geometryStorageMap;
	GeometryStorage* _emptyStorage;
	GeometryMemory*  _emptyGeometryMemory;
	mutable DataStorage _dataStorage;
	int                 _highestUsedStagingMemory = -1;
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
	size_t           _drawIndirectCommandOffset;

	vk::CommandPool _transientCommandPool;
	vk::CommandBuffer _uploadingCommandBuffer;
	vk::CommandPool _precompiledCommandPool;
	vk::CommandBuffer _readTimestampCommandBuffer;
	vk::QueryPool _readTimestampQueryPool;
	vk::Fence _fence;  ///< Fence for general synchronization.

	vk::PipelineCache _pipelineCache;
	vk::ShaderModule _processDrawablesShader;
	vk::PipelineLayout _processDrawablesPipelineLayout;
	vk::Pipeline _processDrawablesPipeline;
	vk::DescriptorPool _descriptorPool;
	vk::DescriptorSetLayout _processDrawablesDescriptorSetLayout;
	vk::DescriptorSet _processDrawablesDescriptorSet;
	vk::DescriptorSetLayout _dataPointerDescriptorSetLayout;
	vk::DescriptorSet _dataPointerDescriptorSet;

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
	static vk::PhysicalDeviceFeatures2 _requiredFeatures;
	friend struct RendererStaticInitializer;

public:

	static Renderer* get();
	static void set(Renderer* r);
	static const vk::PhysicalDeviceFeatures2* requiredFeatures();

	Renderer(bool makeDefault=true);
	Renderer(VulkanDevice& device,VulkanInstance& instance,vk::PhysicalDevice physicalDevice,
	         uint32_t graphicsQueueFamily,bool makeDefault=true);
	~Renderer();
	void init(VulkanDevice& device,VulkanInstance& instance,vk::PhysicalDevice physicalDevice,
	          uint32_t graphicsQueueFamily,bool makeDefault=true);
	void finalize();
	void leakResources();

	size_t beginFrame();  ///< Call this method to mark the beginning of the frame rendering. It returns the frame number assigned to this frame. It is the same number as frameNumber() will return from now on until the next call to beginFrame().
	void beginRecording(vk::CommandBuffer commandBuffer);  ///< Start recording of the command buffer.
	size_t prepareSceneRendering(StateSet& stateSetRoot);
	void recordDrawableProcessing(vk::CommandBuffer commandBuffer,size_t numDrawables);
	void recordSceneRendering(vk::CommandBuffer commandBuffer,StateSet& stateSetRoot,vk::RenderPass renderPass,
	                          vk::Framebuffer framebuffer,const vk::Rect2D& renderArea,
	                          uint32_t clearValueCount, const vk::ClearValue* clearValues);
	void endRecording(vk::CommandBuffer commandBuffer);  ///< Finish recording of the command buffer.
	void endFrame();  ///< Mark the end of frame recording. This is usually called after the command buffer is submitted to gpu for execution.

	VulkanDevice* device() const;
	uint32_t graphicsQueueFamily() const;
	vk::Queue graphicsQueue() const;
	const vk::PhysicalDeviceMemoryProperties& memoryProperties() const;
	size_t standardBufferAlignment() const;
	size_t alignStandardBuffer(size_t offset) const;
	size_t getVertexCapacityForBuffer(const AttribSizeList& attribSizeList,size_t bufferSize) const;

	size_t frameNumber() const;  ///< Returns the frame number of the Renderer. The first frame number is zero and increments for each rendered frame. The initial value is -1 until the first frame rendering starts. The frame number is incremented in beginFrame() and the same value is returned until the next call to beginFrame().
	bool collectFrameInfo() const;  ///< Returns whether frame rendering information is collected.
	void setCollectFrameInfo(bool on, bool useCalibratedTimestamps = false);  ///< Sets whether collecting of frame rendering information will be performed. The method should not be called between beginFrame() and endFrame().
	void setCollectFrameInfo(bool on, bool useCalibratedTimestamps, vk::TimeDomainEXT timestampHostTimeDomain);  ///< Sets whether collecting of frame rendering information will be performed. The method should not be called between beginFrame() and endFrame().
	std::list<FrameInfo> getFrameInfos();  ///< Returns list of FrameInfo for the frames whose collecting of information already completed. Some info is collected as gpu progresses on the frame rendering. The method returns info only about those frames whose collecting of information already finished. To avoid unnecessary memory consumption, unretrieved FrameInfos are deleted after some time. To avoid lost FrameInfos, it is enough to call getFrameInfos() once between each endFrame() and beginFrame() of two consecutive frames.
	double cpuTimestampPeriod() const;  ///< The time period of cpu timestamp begin incremented by 1. The period is given in seconds.
	float gpuTimestampPeriod() const;  ///< The time period of gpu timestamp being incremented by 1. The period is given in seconds.
	uint64_t getCpuTimestamp() const;  ///< Returns the cpu timestamp.
	uint64_t getGpuTimestamp() const;  ///< Returns the gpu timestamp.

	DataStorage& dataStorage() const;
	vk::Buffer drawableBuffer() const;
	size_t drawableBufferSize() const;
	vk::Buffer drawableStagingBuffer() const;
	DrawableGpuData* drawableStagingData() const;
	vk::Buffer matrixListControlBuffer() const;
	vk::Buffer drawIndirectBuffer() const;
	size_t drawIndirectCommandOffset() const;

	vk::PipelineCache pipelineCache() const;
	vk::Pipeline processDrawablesPipeline() const;
	vk::PipelineLayout processDrawablesPipelineLayout() const;
	vk::DescriptorSet processDrawablesDescriptorSet() const;
	vk::DescriptorSet dataPointerDescriptorSet() const;
	vk::CommandPool transientCommandPool() const;
	vk::CommandPool precompiledCommandPool() const;

	GeometryStorage* getOrCreateGeometryStorage(const AttribSizeList& attribSizeList);
	std::map<AttribSizeList,GeometryStorage>& geometryStorageMap();
	GeometryStorage* emptyStorage();
	const GeometryStorage* emptyStorage() const;
	GeometryMemory* emptyGeometryMemory();
	const GeometryMemory* emptyGeometryMemory() const;

	vk::DeviceMemory allocateMemory(vk::Buffer buffer, vk::MemoryPropertyFlags requiredFlags);
	vk::DeviceMemory allocatePointerAccessMemory(vk::Buffer buffer, vk::MemoryPropertyFlags requiredFlags);
	vk::DeviceMemory allocatePointerAccessMemoryNoThrow(vk::Buffer buffer, vk::MemoryPropertyFlags requiredFlags) noexcept;
	void executeCopyOperations();
	void disposeUnusedStagingMemory();

	const std::array<size_t,3>& bufferSizeList() const;

};


}

// inline methods
namespace CadR {
inline Renderer* Renderer::get()  { return _defaultRenderer; }
inline void Renderer::set(Renderer* r)  { _defaultRenderer=r; }
inline const vk::PhysicalDeviceFeatures2* Renderer::requiredFeatures()  { return &_requiredFeatures; }
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
inline DataStorage& Renderer::dataStorage() const  { return _dataStorage; }
inline size_t Renderer::drawableBufferSize() const  { return _drawableBufferSize; }
inline vk::Buffer Renderer::drawableStagingBuffer() const  { return _drawableStagingBuffer; }
inline DrawableGpuData* Renderer::drawableStagingData() const  { return _drawableStagingData; }
inline vk::Buffer Renderer::matrixListControlBuffer() const  { return _matrixListControlBuffer; }
inline vk::Buffer Renderer::drawIndirectBuffer() const  { return _drawIndirectBuffer; }
inline size_t Renderer::drawIndirectCommandOffset() const  { return _drawIndirectCommandOffset; }
inline vk::PipelineCache Renderer::pipelineCache() const  { return _pipelineCache; }
inline vk::Pipeline Renderer::processDrawablesPipeline() const  { return _processDrawablesPipeline; }
inline vk::PipelineLayout Renderer::processDrawablesPipelineLayout() const  { return _processDrawablesPipelineLayout; }
inline vk::DescriptorSet Renderer::processDrawablesDescriptorSet() const  { return _processDrawablesDescriptorSet; }
inline vk::DescriptorSet Renderer::dataPointerDescriptorSet() const  { return _dataPointerDescriptorSet; }
inline vk::CommandPool Renderer::transientCommandPool() const  { return _transientCommandPool; }
inline vk::CommandPool Renderer::precompiledCommandPool() const  { return _precompiledCommandPool; }
inline std::map<AttribSizeList, GeometryStorage>& Renderer::geometryStorageMap()  { return _geometryStorageMap; }
inline GeometryStorage* Renderer::emptyStorage()  { return _emptyStorage; }
inline const GeometryStorage* Renderer::emptyStorage() const  { return _emptyStorage; }
inline GeometryMemory* Renderer::emptyGeometryMemory()  { return _emptyGeometryMemory; }
inline const GeometryMemory* Renderer::emptyGeometryMemory() const  { return _emptyGeometryMemory; }
inline const std::array<size_t,3>& Renderer::bufferSizeList() const  { return _bufferSizeList; }

}

#endif /* CADR_RENDERER_H */
