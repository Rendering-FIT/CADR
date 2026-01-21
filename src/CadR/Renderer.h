#ifndef CADR_RENDERER_HEADER
# define CADR_RENDERER_HEADER

# ifndef CADR_NO_INLINE_FUNCTIONS
#  define CADR_NO_INLINE_FUNCTIONS
#  include <CadR/DataStorage.h>
#  include <CadR/FrameInfo.h>
#  include <CadR/ImageStorage.h>
#  include <CadR/MatrixList.h>
#  include <CadR/StagingManager.h>
#  undef CADR_NO_INLINE_FUNCTIONS
# else
#  include <CadR/DataStorage.h>
#  include <CadR/FrameInfo.h>
#  include <CadR/ImageStorage.h>
#  include <CadR/MatrixList.h>
#  include <CadR/StagingManager.h>
# endif
# include <vulkan/vulkan.hpp>
# include <array>
# include <tuple>

namespace CadR {

class StateSet;
class VulkanDevice;
class VulkanInstance;
struct DrawableGpuData;


class CADR_EXPORT Renderer {
public:
	using RequiredFeaturesStructChain = vk::StructureChain<vk::PhysicalDeviceFeatures2,
		vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan12Features>;
protected:

	VulkanDevice* _device = nullptr;
	uint32_t _graphicsQueueFamily;
	vk::Queue _graphicsQueue;
	vk::PhysicalDeviceMemoryProperties _memoryProperties;
	vk::DeviceSize _standardBufferAlignment;  ///< Memory alignment of a standard buffer. It is used for optimization purposes like putting more small buffers into one large buffer.
	vk::DeviceSize _nonCoherentAtom_addition;  ///< Serves for memory alignment purposes. It is equivalent to PhysicalDeviceLimits::nonCoherentAtomSize-1.
	vk::DeviceSize _nonCoherentAtom_mask;  ///< Serves for memory alignment purposes. It is equivalent to bitwise negation of nonCoherentAtom_addition value.

	vk::Buffer        _drawableBuffer;
	vk::DeviceMemory  _drawableBufferMemory;
	size_t            _drawableBufferSize = 0;
	vk::DeviceAddress _drawableBufferAddress;
	vk::Buffer        _drawableStagingBuffer;
	vk::DeviceMemory  _drawableStagingMemory;
	DrawableGpuData*  _drawableStagingData;
	vk::Buffer        _drawIndirectBuffer;
	vk::DeviceMemory  _drawIndirectMemory;
	vk::DeviceAddress _drawIndirectBufferAddress;
	vk::Buffer        _drawablePointersBuffer;
	vk::DeviceMemory  _drawablePointersMemory;
	vk::DeviceAddress _drawablePointersBufferAddress;

	mutable StagingManager _stagingManager;
	mutable DataStorage _dataStorage;
	size_t _currentFrameUploadBytes = 0;
	size_t _lastFrameUploadBytes = 0;
	mutable ImageStorage _imageStorage;

	vk::CommandPool _transientCommandPool;
	vk::CommandBuffer _uploadingCommandBuffer;
	vk::CommandPool _precompiledCommandPool;
	vk::CommandBuffer _readTimestampCommandBuffer;
	vk::QueryPool _readTimestampQueryPool;
	vk::Fence _fence;  ///< Fence for general synchronization.

	vk::PipelineCache _pipelineCache;
	std::array<vk::ShaderModule,3> _processDrawablesShaderList;
	vk::PipelineLayout _processDrawablesPipelineLayout;
	std::array<vk::Pipeline,3> _processDrawablesPipelineList;
	vk::DescriptorPool _descriptorPool;

	size_t _frameNumber = ~size_t(0);  ///< Monotonically increasing frame number. The first frame is 0. The initial value is -1, marking pre-first frame time.
	double _cpuTimestampPeriod;  ///< The time period of cpu timestamp begin incremented by 1. The period is given in seconds.
	float _gpuTimestampPeriod;  ///< The time period of gpu timestamp being incremented by 1. The period is given in seconds.
	bool _collectFrameInfo = false;  ///< True if frame timing information collecting is enabled.
	bool _useCalibratedTimestamps;  ///< True if use of calibrated timestamps is enabled. It requires VK_EXT_calibrated_timestamps Vulkan extension to be present and enabled.
	vk::TimeDomainEXT _timestampHostTimeDomain;  ///< Time domain used for the cpu timestamps.
	vk::QueryPool _frameInfoTimestampPool;  ///< QueryPool for timestamps that are collected during frame rendering.
	struct FrameInfoCollector : public FrameInfo {  ///< The structure is used during collection of FrameInfo.
		uint32_t numTimestamps;  ///< Number of timestamps written so far to the timestampPool.
		bool beingCollected;  ///< Flag indicating that the structure is in use and data are being collected.
	};
	FrameInfoCollector _inProgressFrameInfo;  ///< Frame info structure used during frame rendering to collect performance data.
	FrameInfo _completedFrameInfo;  ///< FrameInfo structure containing info about the recently finished frame. The structure contains invalid frame (frameNumber set to -1) after calling endFrame(). It is set to valid values by getFrameInfo() function when all the data including timestamp queries becomes available. The data of previous frame are always available after calling beginFrame(), because it waits for finishing of the previous frame.

	static Renderer* _defaultRenderer;
	static RequiredFeaturesStructChain _requiredFeatures;
	friend struct RendererStaticInitializer;

public:

	// public static variables
	static constexpr const size_t smallMemorySize = 64 << 10;  // 64KiB
	static constexpr const size_t mediumMemorySize = 2 << 20;  // 2MiB
	static constexpr const size_t largeMemorySize = 32 << 20;  // 32MiB

	// general static functions
	static inline Renderer& get();
	static inline void set(Renderer& r) noexcept;
	static inline const vk::PhysicalDeviceFeatures2& requiredFeatures();
	static inline const RequiredFeaturesStructChain& requiredFeaturesStructChain();

	// deleted constructors and operators
	Renderer(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	Renderer& operator=(Renderer&&) = delete;

	// construction, initialization and destruction
	Renderer(bool makeDefault = true) noexcept;
	Renderer(VulkanDevice& device, VulkanInstance& instance, vk::PhysicalDevice physicalDevice,
	         uint32_t graphicsQueueFamily, bool makeDefault = true);
	~Renderer();
	void init(VulkanDevice& device, VulkanInstance& instance, vk::PhysicalDevice physicalDevice,
	          uint32_t graphicsQueueFamily, bool makeDefault = true);
	void finalize();
	void leakResources();

	// frame API
	size_t beginFrame();  ///< Call this method to mark the beginning of the frame rendering. It returns the frame number assigned to this frame. It is the same number as frameNumber() will return from now on until the next call to beginFrame().
	void beginRecording(vk::CommandBuffer commandBuffer);  ///< Start recording of the command buffer.
	size_t prepareSceneRendering(StateSet& stateSetRoot);
	void recordDrawableProcessing(vk::CommandBuffer commandBuffer, size_t numDrawables);
	void recordSceneRendering(vk::CommandBuffer commandBuffer, StateSet& stateSetRoot, vk::RenderPass renderPass,
	                          vk::Framebuffer framebuffer, const vk::Rect2D& renderArea,
	                          uint32_t clearValueCount, const vk::ClearValue* clearValues);
	void endRecording(vk::CommandBuffer commandBuffer);  ///< Finish recording of the command buffer.
	void endFrame();  ///< Mark the end of frame recording. This is usually called after the command buffer is submitted to gpu for execution.

	// getters
	inline VulkanDevice& device() const;
	inline uint32_t graphicsQueueFamily() const;
	inline vk::Queue graphicsQueue() const;
	inline const vk::PhysicalDeviceMemoryProperties& memoryProperties() const;
	inline size_t standardBufferAlignment() const;
	inline size_t alignStandardBuffer(size_t offset) const;

	// frame related API
	inline size_t frameNumber() const noexcept;  ///< Returns the frame number of the Renderer. The first frame number is zero and increments for each rendered frame. The initial value is -1 until the first frame rendering starts. The frame number is incremented in beginFrame() and the same value is returned until the next call to beginFrame().
	inline bool collectFrameInfo() const;  ///< Returns whether frame rendering information is collected.
	void setCollectFrameInfo(bool on, bool useCalibratedTimestamps = false);  ///< Sets whether collecting of frame rendering information will be performed. The method should not be called between beginFrame() and endFrame().
	void setCollectFrameInfo(bool on, bool useCalibratedTimestamps, vk::TimeDomainEXT timestampHostTimeDomain);  ///< Sets whether collecting of frame rendering information will be performed. The method should not be called between beginFrame() and endFrame().
	const FrameInfo& getFrameInfo();  ///< Returns FrameInfo containing the info about the recently finished frame. After calling endFrame(), it is initialized to invalid frame (frameNumber set to -1). It is set to valid values by getFrameInfo() function when all the data including timestamp queries becomes available. The data of previous frame are always available after calling beginFrame(), because it waits for finishing of the previous frame.
	inline const FrameInfo& getCurrentFrameInfo();  ///< Returns current FrameInfo being collected just now. It might be useful to read cpu timing information collected between beginFrame() and endFrame(). Some members becomes available only from particular moments of frame rendering. Collecting of frame info must be switched on (see setCollectFrameInfo()).
	inline double cpuTimestampPeriod() const;  ///< The time period of cpu timestamp begin incremented by 1. The period is given in seconds.
	inline float gpuTimestampPeriod() const;  ///< The time period of gpu timestamp being incremented by 1. The period is given in seconds.
	std::tuple<uint64_t, uint64_t> getCpuAndGpuTimestamps() const;
	uint64_t getCpuTimestamp() const;  ///< Returns the cpu timestamp.
	uint64_t getGpuTimestamp() const;  ///< Returns the gpu timestamp.

	// storage
	inline DataStorage& dataStorage() const;
	inline ImageStorage& imageStorage() const;
	inline StagingManager& stagingManager() const;

	// data and buffers
	inline vk::Buffer drawableBuffer() const;
	inline size_t drawableBufferSize() const;
	inline vk::Buffer drawableStagingBuffer() const;
	inline DrawableGpuData* drawableStagingData() const;
	inline vk::Buffer drawIndirectBuffer() const;
	inline vk::DeviceAddress drawIndirectBufferAddress() const;
	inline vk::Buffer drawablePointersBuffer() const;
	inline vk::DeviceAddress drawablePointersBufferAddress() const;

	// pipelines and command pools
	inline vk::PipelineCache pipelineCache() const;
	inline vk::Pipeline processDrawablesPipeline(size_t handleLevel) const;
	inline vk::PipelineLayout processDrawablesPipelineLayout() const;
	inline vk::CommandPool transientCommandPool() const;
	inline vk::CommandPool precompiledCommandPool() const;

	// memory
	inline vk::DeviceMemory allocateMemoryType(size_t size, uint32_t memoryTypeIndex);
	vk::DeviceMemory allocateMemoryTypeNoThrow(size_t size, uint32_t memoryTypeIndex) noexcept;
	inline std::tuple<vk::DeviceMemory, uint32_t> allocateMemory(vk::Buffer buffer, vk::MemoryPropertyFlags requiredFlags);
	std::tuple<vk::DeviceMemory, uint32_t> allocateMemory(size_t size, uint32_t memoryTypeBits, vk::MemoryPropertyFlags requiredFlags);
	inline std::tuple<vk::DeviceMemory, uint32_t> allocatePointerAccessMemory(vk::Buffer buffer, vk::MemoryPropertyFlags requiredFlags);
	std::tuple<vk::DeviceMemory, uint32_t> allocatePointerAccessMemory(size_t size, uint32_t memoryTypeBits, vk::MemoryPropertyFlags requiredFlags);
	inline std::tuple<vk::DeviceMemory, uint32_t> allocatePointerAccessMemoryNoThrow(vk::Buffer buffer, vk::MemoryPropertyFlags requiredFlags) noexcept;
	std::tuple<vk::DeviceMemory, uint32_t> allocatePointerAccessMemoryNoThrow(size_t size, uint32_t memoryTypeBits, vk::MemoryPropertyFlags requiredFlags) noexcept;
	void executeCopyOperations();

	static constexpr uint32_t drawablePointersRecordSize = 4 * sizeof(uint64_t);

};


}

#endif


// inline methods
#if !defined(CADR_RENDERER_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_RENDERER_INLINE_FUNCTIONS
# define CADR_NO_INLINE_FUNCTIONS
# include <CadR/VulkanDevice.h>
# undef CADR_NO_INLINE_FUNCTIONS
# include <cassert>
namespace CadR {

inline Renderer& Renderer::get()  { return *_defaultRenderer; }
inline void Renderer::set(Renderer& r) noexcept  { _defaultRenderer = &r; }
inline const vk::PhysicalDeviceFeatures2& Renderer::requiredFeatures()  { return _requiredFeatures.get<vk::PhysicalDeviceFeatures2>(); }
inline const Renderer::RequiredFeaturesStructChain& Renderer::requiredFeaturesStructChain()  { return _requiredFeatures; }
inline VulkanDevice& Renderer::device() const  { assert(_device && "Renderer::device(): Renderer must be initialized with valid VulkanDevice to call this function."); return *_device; }
inline uint32_t Renderer::graphicsQueueFamily() const  { return _graphicsQueueFamily; }
inline vk::Queue Renderer::graphicsQueue() const  { return _graphicsQueue; }
inline const vk::PhysicalDeviceMemoryProperties& Renderer::memoryProperties() const  { return _memoryProperties; }
inline size_t Renderer::standardBufferAlignment() const  { return _standardBufferAlignment; }
inline size_t Renderer::alignStandardBuffer(size_t offset) const  { size_t a=_standardBufferAlignment-1; return (offset+a)&(~a); }
inline size_t Renderer::frameNumber() const noexcept  { return _frameNumber; }
inline bool Renderer::collectFrameInfo() const  { return _collectFrameInfo; }
inline const FrameInfo& Renderer::getCurrentFrameInfo()  { return _inProgressFrameInfo; }
inline double Renderer::cpuTimestampPeriod() const  { return _cpuTimestampPeriod; }
inline float Renderer::gpuTimestampPeriod() const  { return _gpuTimestampPeriod; }
inline DataStorage& Renderer::dataStorage() const  { return _dataStorage; }
inline ImageStorage& Renderer::imageStorage() const  { return _imageStorage; }
inline StagingManager& Renderer::stagingManager() const  { return _stagingManager; }
inline vk::Buffer Renderer::drawableBuffer() const  { return _drawableBuffer; }
inline size_t Renderer::drawableBufferSize() const  { return _drawableBufferSize; }
inline vk::Buffer Renderer::drawableStagingBuffer() const  { return _drawableStagingBuffer; }
inline DrawableGpuData* Renderer::drawableStagingData() const  { return _drawableStagingData; }
inline vk::Buffer Renderer::drawIndirectBuffer() const  { return _drawIndirectBuffer; }
inline vk::DeviceAddress Renderer::drawIndirectBufferAddress() const  { return _drawIndirectBufferAddress; }
inline vk::Buffer Renderer::drawablePointersBuffer() const  { return _drawablePointersBuffer; }
inline vk::DeviceAddress Renderer::drawablePointersBufferAddress() const  { return _drawablePointersBufferAddress; }
inline vk::PipelineCache Renderer::pipelineCache() const  { return _pipelineCache; }
inline vk::Pipeline Renderer::processDrawablesPipeline(size_t handleLevel) const  { return _processDrawablesPipelineList[handleLevel-1]; }
inline vk::PipelineLayout Renderer::processDrawablesPipelineLayout() const  { return _processDrawablesPipelineLayout; }
inline vk::CommandPool Renderer::transientCommandPool() const  { return _transientCommandPool; }
inline vk::CommandPool Renderer::precompiledCommandPool() const  { return _precompiledCommandPool; }
inline vk::DeviceMemory Renderer::allocateMemoryType(size_t size, uint32_t memoryTypeIndex)  { vk::DeviceMemory m = allocateMemoryTypeNoThrow(size, memoryTypeIndex); if(!m) throw std::runtime_error("Cannot allocate memory of specified memory type."); return m; }
inline std::tuple<vk::DeviceMemory, uint32_t> Renderer::allocateMemory(vk::Buffer buffer, vk::MemoryPropertyFlags requiredFlags)  { vk::MemoryRequirements r = _device->getBufferMemoryRequirements(buffer); return allocateMemory(r.size, r.memoryTypeBits, requiredFlags); }
inline std::tuple<vk::DeviceMemory, uint32_t> Renderer::allocatePointerAccessMemory(vk::Buffer buffer, vk::MemoryPropertyFlags requiredFlags)  { vk::MemoryRequirements r = _device->getBufferMemoryRequirements(buffer); return allocatePointerAccessMemory(r.size, r.memoryTypeBits, requiredFlags); }
inline std::tuple<vk::DeviceMemory, uint32_t> Renderer::allocatePointerAccessMemoryNoThrow(vk::Buffer buffer, vk::MemoryPropertyFlags requiredFlags) noexcept  { vk::MemoryRequirements r = _device->getBufferMemoryRequirements(buffer); return allocatePointerAccessMemoryNoThrow(r.size, r.memoryTypeBits, requiredFlags); }

}
#endif
