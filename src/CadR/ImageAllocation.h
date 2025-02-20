#ifndef CADR_IMAGE_ALLOCATION_HEADER
# define CADR_IMAGE_ALLOCATION_HEADER

# ifndef CADR_NO_INLINE_FUNCTIONS
#  define CADR_NO_INLINE_FUNCTIONS
#  include <CadR/StagingBuffer.h>
#  undef CADR_NO_INLINE_FUNCTIONS
# else
#  include <CadR/StagingBuffer.h>
# endif
# include <vulkan/vulkan.hpp>
# include <boost/intrusive/list.hpp>
# include <functional>

namespace CadR {

class ImageMemory;
class ImageStorage;
class Renderer;
class VulkanDevice;
struct DataAllocationRecord;
struct ImageAllocationRecord;



struct CADR_EXPORT CopyRecord
{
	union {
		DataAllocationRecord* dataAllocationRecord;
		ImageAllocationRecord* imageAllocationRecord;
	};
	vk::Image imageToDestroy;
	uint32_t referenceCounter;  //< Number of ImageMemory::BufferToImageUploads that points to this CopyRecord.
	uint32_t copyOpCounter;  //< Counter of in-progress copy operations. The number is incremented when ImageMemory::BufferToImageUpload is recorded to a CommandBufer and decremented after execution of command buffer is finished or when command buffer is deemed to not be executed.
};


struct CADR_EXPORT ImageChangedCallback
{
	std::function<void(vk::Image newImage)> imageChanged;

	boost::intrusive::list_member_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink>
	> _callbackHook;  ///< List hook of ImageAllocationRecord::imageChangedCallbackList.
};


struct CADR_EXPORT ImageAllocationRecord
{
	uint64_t memoryOffset;
	size_t size;
	ImageMemory* imageMemory;
	ImageAllocationRecord** recordPointer;
	CopyRecord* copyRecord;
	vk::Image image;
	vk::ImageCreateInfo imageCreateInfo;

	boost::intrusive::list<
		ImageChangedCallback,
		boost::intrusive::member_hook<
			ImageChangedCallback,
			boost::intrusive::list_member_hook<
				boost::intrusive::link_mode<boost::intrusive::auto_unlink>>,
			&ImageChangedCallback::_callbackHook>,
		boost::intrusive::constant_time_size<false>
	> imageChangedCallbackList;

	inline void init(uint64_t memoryOffset, size_t size, ImageMemory* m,
			ImageAllocationRecord** recordPointer, CopyRecord* copyRecord) noexcept;
	void releaseHandles(VulkanDevice& device) noexcept;
	void callImageChangedCallbacks();
	static ImageAllocationRecord nullRecord;
};


/** \brief DataAllocation represents single piece of allocated memory.
 *
 *  The allocated memory is suballocated from DataMemory objects and managed using DataStorage.
 *
 *  \sa DataMemory, DataStorage
 */
class CADR_EXPORT ImageAllocation {
protected:
	ImageAllocationRecord* _record;
	friend ImageMemory;
	friend ImageStorage;
	friend StagingBuffer;
public:

	// construction and destruction
	inline ImageAllocation(nullptr_t) noexcept;
	inline ImageAllocation(ImageStorage& storage) noexcept;
	inline ImageAllocation(ImageAllocation&&) noexcept;  ///< Move constructor.
	ImageAllocation(const ImageAllocation&) = delete;  ///< No copy constructor.
	inline ~ImageAllocation() noexcept;  ///< Destructor.

	// operators
	inline ImageAllocation& operator=(ImageAllocation&&) noexcept;  ///< Move assignment operator.
	ImageAllocation& operator=(const ImageAllocation&) = delete;  ///< No copy assignment.

	// alloc and free
	inline void init(ImageStorage& storage);
	inline void alloc(size_t numBytes, size_t alignment, uint32_t memoryTypeBits,
			vk::MemoryPropertyFlags requiredFlags, vk::Image image, const vk::ImageCreateInfo& imageCreateInfo);
		//< Allocates memory for the image.
		//< If ImageAllocation already contains valid alocation, it is freed before the new allocation is attempted.
		//< If there is not enough memory or an error occured, an exception is thrown.
	inline void alloc(vk::MemoryPropertyFlags requiredFlags,
			const vk::ImageCreateInfo& imageCreateInfo, VulkanDevice& device);
		//< Allocates memory for the image and creates the image.
		//< If ImageAllocation already contains valid alocation, it is freed before the new allocation is attempted.
		//< If there is not enough memory or an error occured, an exception is thrown.
	inline void free() noexcept;

	// getters
	inline uint64_t memoryOffset() const;
	inline size_t size() const;
	inline ImageMemory& imageMemory() const;
	inline ImageStorage& imageStorage() const;
	inline Renderer& renderer() const;
	inline vk::Image image() const;
	inline const vk::ImageCreateInfo& imageCreateInfo() const;

	// data update
	inline StagingBuffer createStagingBuffer(size_t numBytes, size_t alignment);
	inline void submit(StagingBuffer& stagingBuffer, vk::ImageLayout oldLayout, vk::ImageLayout copyLayout,
			vk::ImageLayout newLayout, vk::PipelineStageFlags newLayoutBarrierStageFlags,
			vk::AccessFlags newLayoutBarrierAccessFlags, const vk::BufferImageCopy& region, size_t dataSize);
	inline void submit(StagingBuffer& stagingBuffer, vk::ImageLayout oldLayout, vk::ImageLayout copyLayout,
			vk::ImageLayout newLayout, vk::PipelineStageFlags newLayoutBarrierStageFlags,
			vk::AccessFlags newLayoutBarrierAccessFlags, vk::Extent2D imageExtent, size_t dataSize);
	void upload(const void* ptr, size_t numBytes);

};


}

#endif


// inline methods
#if !defined(CADR_IMAGE_ALLOCATION_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_IMAGE_ALLOCATION_INLINE_FUNCTIONS
# define CADR_NO_INLINE_FUNCTIONS
# include <CadR/VulkanDevice.h>
# undef CADR_NO_INLINE_FUNCTIONS
# include <CadR/ImageMemory.h>
# include <CadR/ImageStorage.h>
# include <CadR/Renderer.h>
# include <CadR/StagingBuffer.h>
namespace CadR {

inline void ImageAllocationRecord::init(uint64_t memoryOffset, size_t size, ImageMemory* m, ImageAllocationRecord** recordPointer, CopyRecord* copyRecord) noexcept  { this->memoryOffset = memoryOffset; this->size = size; imageMemory = m; this->recordPointer = recordPointer; this->copyRecord = copyRecord; }
inline void ImageAllocationRecord::callImageChangedCallbacks()  { for(auto& li : imageChangedCallbackList) li.imageChanged(image); }

inline ImageAllocation::ImageAllocation(nullptr_t) noexcept : _record(&ImageAllocationRecord::nullRecord)  {}
inline ImageAllocation::ImageAllocation(ImageStorage& storage) noexcept  { _record = storage.zeroSizeAllocationRecord(); }
inline ImageAllocation::ImageAllocation(ImageAllocation&& other) noexcept : _record(other._record)  { other._record->recordPointer=&this->_record; other._record=_record->imageMemory->imageStorage().zeroSizeAllocationRecord(); }
inline ImageAllocation::~ImageAllocation() noexcept  { free(); }
inline ImageAllocation& ImageAllocation::operator=(ImageAllocation&& rhs) noexcept  { free(); _record=rhs._record; rhs._record->recordPointer=&this->_record; rhs._record=_record->imageMemory->imageStorage().zeroSizeAllocationRecord(); return *this; }

inline void ImageAllocation::init(ImageStorage& storage)  { if(_record->size!=0) { ImageMemory* im=_record->imageMemory; _record->releaseHandles(im->imageStorage().renderer().device()); im->freeInternal(_record); } _record=storage.zeroSizeAllocationRecord(); }
inline void ImageAllocation::alloc(size_t numBytes, size_t alignment, uint32_t memoryTypeBits, vk::MemoryPropertyFlags requiredFlags, vk::Image image, const vk::ImageCreateInfo& imageCreateInfo) { ImageStorage& storage=_record->imageMemory->imageStorage(); storage.alloc(*this, numBytes, alignment, memoryTypeBits, requiredFlags, image, imageCreateInfo); }
inline void ImageAllocation::alloc(vk::MemoryPropertyFlags requiredFlags, const vk::ImageCreateInfo& imageCreateInfo, VulkanDevice& device)  { vk::Image image=device.createImage(imageCreateInfo); try { vk::MemoryRequirements memoryRequirements=device.getImageMemoryRequirements(image); alloc(memoryRequirements.size, memoryRequirements.alignment, memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal, image, imageCreateInfo); } catch(...) { device.destroy(image); throw; } }
inline void ImageAllocation::free() noexcept { if(_record->size==0) return; ImageMemory* im=_record->imageMemory; auto zeroRecord=im->imageStorage().zeroSizeAllocationRecord(); _record->releaseHandles(im->imageStorage().renderer().device()); im->freeInternal(_record); _record=zeroRecord; }

inline uint64_t ImageAllocation::memoryOffset() const  { return _record->memoryOffset; }
inline size_t ImageAllocation::size() const  { return _record->size; }
inline ImageMemory& ImageAllocation::imageMemory() const  { return *_record->imageMemory; }
inline ImageStorage& ImageAllocation::imageStorage() const  { return _record->imageMemory->imageStorage(); }
inline Renderer& ImageAllocation::renderer() const  { return _record->imageMemory->imageStorage().renderer(); }
inline vk::Image ImageAllocation::image() const  { return _record->image; }
inline const vk::ImageCreateInfo& ImageAllocation::imageCreateInfo() const  { return _record->imageCreateInfo; }

inline StagingBuffer ImageAllocation::createStagingBuffer(size_t numBytes, size_t alignment)  { return StagingBuffer(_record->imageMemory->imageStorage(), numBytes, alignment); }
inline void ImageAllocation::submit(StagingBuffer& stagingBuffer, vk::ImageLayout oldLayout, vk::ImageLayout copyLayout, vk::ImageLayout newLayout, vk::PipelineStageFlags newLayoutBarrierStageFlags, vk::AccessFlags newLayoutBarrierAccessFlags, const vk::BufferImageCopy& region, size_t dataSize)  { stagingBuffer.submit(*this, oldLayout, copyLayout, newLayout, newLayoutBarrierStageFlags, newLayoutBarrierAccessFlags, region, dataSize); }
inline void ImageAllocation::submit(StagingBuffer& stagingBuffer, vk::ImageLayout oldLayout, vk::ImageLayout copyLayout, vk::ImageLayout newLayout, vk::PipelineStageFlags newLayoutBarrierStageFlags, vk::AccessFlags newLayoutBarrierAccessFlags, vk::Extent2D imageExtent, size_t dataSize)  { stagingBuffer.submit(*this, oldLayout, copyLayout, newLayout, newLayoutBarrierStageFlags, newLayoutBarrierAccessFlags, imageExtent, dataSize); }

}
#endif
