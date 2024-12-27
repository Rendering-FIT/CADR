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

namespace CadR {

class ImageMemory;
class ImageStorage;
class Renderer;
class VulkanDevice;



struct CADR_EXPORT CopyRecord
{
	union {
		DataAllocation* dataAllocation;
		HandlelessAllocation* handlelessAllocation;
		ImageAllocation* imageAllocation;
	};
	vk::Image imageToDestroy;
	uint32_t referenceCounter;
	uint32_t copyOpCounter;
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

	void init(uint64_t memoryOffset, size_t size, ImageMemory* m, ImageAllocationRecord** recordPointer,
	          CopyRecord* copyRecord) noexcept;
	void releaseHandles(VulkanDevice& device) noexcept;
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
	ImageAllocation(nullptr_t) noexcept;
	ImageAllocation(ImageStorage& storage) noexcept;
	ImageAllocation(ImageAllocation&&) noexcept;  ///< Move constructor.
	ImageAllocation(const ImageAllocation&) = delete;  ///< No copy constructor.
	~ImageAllocation() noexcept;  ///< Destructor.

	// operators
	ImageAllocation& operator=(ImageAllocation&&) noexcept;  ///< Move assignment operator.
	ImageAllocation& operator=(const ImageAllocation&) = delete;  ///< No copy assignment.

	// alloc and free
	void init(ImageStorage& storage);
	void alloc(size_t numBytes, size_t alignment, uint32_t memoryTypeBits, vk::MemoryPropertyFlags requiredFlags,
	           vk::Image image, const vk::ImageCreateInfo& imageCreateInfo);
		//< Allocates memory for the image.
		//< If ImageAllocation already contains valid alocation, it is freed before the new allocation is attempted.
		//< If there is not enough memory or an error occured, an exception is thrown.
	void alloc(vk::MemoryPropertyFlags requiredFlags, const vk::ImageCreateInfo& imageCreateInfo, VulkanDevice& device);
		//< Allocates memory for the image and creates the image.
		//< If ImageAllocation already contains valid alocation, it is freed before the new allocation is attempted.
		//< If there is not enough memory or an error occured, an exception is thrown.
	void free() noexcept;

	// getters
	uint64_t memoryOffset() const;
	size_t size() const;
	ImageMemory& imageMemory() const;
	ImageStorage& imageStorage() const;
	Renderer& renderer() const;
	vk::Image image() const;
	const vk::ImageCreateInfo& imageCreateInfo() const;

	// data update
	StagingBuffer createStagingBuffer(size_t numBytes, size_t alignment);
	void submit(StagingBuffer& stagingBuffer, vk::ImageLayout oldLayout, vk::ImageLayout copyLayout,
	            vk::ImageLayout newLayout, vk::PipelineStageFlags newLayoutBarrierStageFlags,
	            vk::AccessFlags newLayoutBarrierAccessFlags, const vk::BufferImageCopy& region);
	void submit(StagingBuffer& stagingBuffer, vk::ImageLayout oldLayout, vk::ImageLayout copyLayout,
	            vk::ImageLayout newLayout, vk::PipelineStageFlags newLayoutBarrierStageFlags,
	            vk::AccessFlags newLayoutBarrierAccessFlags, vk::Extent2D imageExtent);
	void upload(const void* ptr, size_t numBytes);

};


}

#endif


// inline methods
#if !defined(CADR_IMAGE_ALLOCATION_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_IMAGE_ALLOCATION_INLINE_FUNCTIONS
# define CADR_NO_INLINE_FUNCTIONS
# include <CadR/ImageStorage.h>
# include <CadR/Renderer.h>
# include <CadR/VulkanDevice.h>
# undef CADR_NO_INLINE_FUNCTIONS
namespace CadR {

inline void ImageAllocationRecord::init(uint64_t memoryOffset, size_t size, ImageMemory* m, ImageAllocationRecord** recordPointer, CopyRecord* copyRecord) noexcept  { this->memoryOffset = memoryOffset; this->size = size; imageMemory = m; this->recordPointer = recordPointer; this->copyRecord = copyRecord; }
inline void ImageAllocationRecord::releaseHandles(VulkanDevice& device) noexcept  { if(copyRecord && copyRecord->copyOpCounter>=1) copyRecord->imageToDestroy=image; else device.destroy(image); image=nullptr; }

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
inline void ImageAllocation::submit(StagingBuffer& stagingBuffer, vk::ImageLayout oldLayout, vk::ImageLayout copyLayout, vk::ImageLayout newLayout, vk::PipelineStageFlags newLayoutBarrierStageFlags, vk::AccessFlags newLayoutBarrierAccessFlags, const vk::BufferImageCopy& region)  { stagingBuffer.submit(*this, oldLayout, copyLayout, newLayout, newLayoutBarrierStageFlags, newLayoutBarrierAccessFlags, region); }
inline void ImageAllocation::submit(StagingBuffer& stagingBuffer, vk::ImageLayout oldLayout, vk::ImageLayout copyLayout, vk::ImageLayout newLayout, vk::PipelineStageFlags newLayoutBarrierStageFlags, vk::AccessFlags newLayoutBarrierAccessFlags, vk::Extent2D imageExtent)  { stagingBuffer.submit(*this, oldLayout, copyLayout, newLayout, newLayoutBarrierStageFlags, newLayoutBarrierAccessFlags, imageExtent); }

}
#endif
