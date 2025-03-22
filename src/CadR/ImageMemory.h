#ifndef CADR_IMAGE_MEMORY_HEADER
# define CADR_IMAGE_MEMORY_HEADER

# ifndef CADR_NO_INLINE_FUNCTIONS
#  define CADR_NO_INLINE_FUNCTIONS
#  include <CadR/CircularAllocationMemory.h>
#  include <CadR/ImageAllocation.h>
#  include <CadR/StagingMemory.h>
#  undef CADR_NO_INLINE_FUNCTIONS
# else
#  include <CadR/CircularAllocationMemory.h>
#  include <CadR/ImageAllocation.h>
#  include <CadR/StagingMemory.h>
# endif
# include <vulkan/vulkan.hpp>
# include <boost/intrusive/list.hpp>
# include <list>

namespace CadR {

class ImageStorage;
class VulkanDevice;

inline constexpr const size_t ImageRecordsPerBlock = 32;


/** \brief
 *  DataMemory class provides GPU memory functionality
 *  for allocating and storing user data.
 *  DataMemory class is usually used together with DataAllocation
 *  and DataStorage classes.
 *
 *  All the data of DataMemory are stored in single vk::Buffer.
 *  The buffer is allocated during DataMemory construction
 *  and cannot change its size afterwards. If you need more
 *  space, allocate another DataMemory object. Optionally,
 *  you might copy the content of the old DataMemory object
 *  into new one and delete the old object.
 *
 *  \sa DataStorage, DataAllocation
 */
class CADR_EXPORT ImageMemory : public CircularAllocationMemory<ImageAllocationRecord, ImageRecordsPerBlock> {
protected:

	ImageStorage* _imageStorage;  ///< ImageStorage that owns this ImageMemory.
	vk::DeviceMemory _memory;
	uint32_t _memoryTypeIndex = ~uint32_t(0);

	struct BufferToImageUpload {
		CopyRecord* copyRecord;
		StagingMemory* stagingMemory;

		vk::Image dstImage;
		vk::ImageLayout oldLayout;
		vk::ImageLayout copyLayout;
		vk::ImageLayout newLayout;
		vk::PipelineStageFlags newLayoutBarrierDstStages;
		vk::AccessFlags newLayoutBarrierDstAccessFlags;
		uint32_t regionCount = 1;
		vk::BufferImageCopy region;
		vk::BufferImageCopy* regionList = nullptr;
		size_t dataSize;

		size_t record(VulkanDevice& device, vk::CommandBuffer commandBuffer);
		inline void allocRegionList(size_t n);
		inline BufferToImageUpload(StagingMemory* stagingMemory, CopyRecord* copyRecord, vk::Image dstImage,
				vk::ImageLayout oldLayout, vk::ImageLayout copyLayout, vk::ImageLayout newLayout,
				vk::PipelineStageFlags newLayoutBarrierDstStages, vk::AccessFlags newLayoutBarrierDstAccessFlags,
				vk::BufferImageCopy region, size_t dataSize);
		inline BufferToImageUpload(StagingMemory* stagingMemory, CopyRecord* copyRecord, vk::Image dstImage,
				vk::ImageLayout oldLayout, vk::ImageLayout copyLayout, vk::ImageLayout newLayout,
				vk::PipelineStageFlags newLayoutBarrierDstStages, vk::AccessFlags newLayoutBarrierDstAccessFlags,
				uint32_t regionCount, std::unique_ptr<vk::BufferImageCopy[]>&& regionList, size_t dataSize);
		inline ~BufferToImageUpload();
	};
	std::list<BufferToImageUpload> _bufferToImageUploadList;

	struct UploadInProgress {
		boost::intrusive::list_member_hook<
			boost::intrusive::link_mode<boost::intrusive::auto_unlink>
		> _uploadInProgressListHook;  ///< List hook of StagingManager::*List.
		std::vector<std::tuple<CopyRecord*, StagingMemory*>> copyRecordList;
	};
	using UploadInProgressList =
		boost::intrusive::list<
			UploadInProgress,
			boost::intrusive::member_hook<
				UploadInProgress,
				boost::intrusive::list_member_hook<
					boost::intrusive::link_mode<boost::intrusive::auto_unlink>>,
				&UploadInProgress::_uploadInProgressListHook>,
			boost::intrusive::constant_time_size<false>
		>;
	UploadInProgressList _uploadInProgressList;

	friend StagingBuffer;

	inline ImageMemory(ImageStorage& imageStorage);

public:

	// construction and destruction
	static ImageMemory* tryCreate(ImageStorage& imageStorage, size_t size, uint32_t memoryTypeIndex);  ///< It attempts to create ImageMemory. If failure occurs during memory allocation, it does not throw but returns null.
	ImageMemory(ImageStorage& imageStorage, size_t size, uint32_t memoryTypeIndex);  ///< Allocates ImageMemory, including underlying Vulkan DeviceMemory. In the case of failure, exception is thrown. In such case, all the resources including ImageMemory object itself are correctly released.
	ImageMemory(ImageStorage& imageStorage, vk::DeviceMemory memory, size_t size, uint32_t memoryTypeIndex);  ///< Allocates ImageMemory object while using memory provided by parameter. The memory must be valid non-null handle.
	ImageMemory(ImageStorage& imageStorage, nullptr_t);  ///< Allocates ImageMemory object initializing memory handle to null and sets size to zero.
	~ImageMemory();  ///< Destructor.

	// deleted constructors and operators
	ImageMemory() = delete;
	ImageMemory(const ImageMemory&) = delete;
	ImageMemory& operator=(const ImageMemory&) = delete;
	ImageMemory(ImageMemory&&) = delete;
	ImageMemory& operator=(ImageMemory&&) = delete;

	// getters
	inline ImageStorage& imageStorage() const;
	inline size_t size() const;
	inline vk::DeviceMemory memory() const;
	inline size_t usedBytes() const;

	// low-level allocation functions
	// (mostly for internal use)
	inline bool alloc(ImageAllocation& a, size_t numBytes, size_t alignment,
			vk::Image image, const vk::ImageCreateInfo& imageCreateInfo);
		//< Allocates memory for the image and Vulkan handles.
		//< If ImageAllocation already contains valid alocation, it is freed before the new allocation is attempted.
		//< It returns true in the case of success. False is returned if there is not enough free space.
		//< In the case of other errors, exceptions are thrown, such as bad_alloc or Vulkan exceptions.
	bool allocInternal(ImageAllocationRecord*& recPtr, size_t numBytes, size_t alignment);
		//< Allocates memory for the image but does not allocate Vulkan handles.
		//< The recPtr must point to ImageAllocation::_record variable that will be overwritten - e.g. it might be null or unitialized memory, etc.
		//< It should not contain valid ImageAllocationRecord pointer because the pointer will not be freed. Returns true in the case of success.
		//< It returns false if there is not enough free space. The function throws in the case of error, such as bad_alloc.
		//< If false is returned or exception is thrown, variable pointed by recPtr stays intact.
	inline void freeInternal(ImageAllocationRecord*& recPtr) noexcept;
		//< Frees the memory allocation of the image. Vulkan handles are not in the scope of this function and must be dealt with elsewhere.
		//< The recPtr must be reference to a valid ImageAllocation::_record pointer and it must not be ImageStorage::zeroSizeAllocationRecord() or similar value.
		//< After the completion, ImageAllocation::_record pointer is invalid and must not be dereferenced. Its value is not replaced by ImageStorage::zeroSizeAllocationRecord() record.

	// data upload
	[[nodiscard]] std::tuple<void*,size_t> recordUploads(vk::CommandBuffer);
	void uploadDone(void*) noexcept;

#if 0
	void cancelAllAllocations();
#endif

};


}

#endif


// inline methods
#if !defined(CADR_IMAGE_MEMORY_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_IMAGE_MEMORY_INLINE_FUNCTIONS
# include <CadR/StagingMemory.h>
namespace CadR {

inline void ImageMemory::BufferToImageUpload::allocRegionList(size_t n)  { delete[] regionList; regionList = new vk::BufferImageCopy[n]; }
inline ImageMemory::BufferToImageUpload::BufferToImageUpload(StagingMemory* stagingMemory, CopyRecord* copyRecord, vk::Image dstImage,
	vk::ImageLayout oldLayout, vk::ImageLayout copyLayout, vk::ImageLayout newLayout, vk::PipelineStageFlags newLayoutBarrierDstStages,
	vk::AccessFlags newLayoutBarrierDstAccessFlags, vk::BufferImageCopy region, size_t dataSize) :
		copyRecord(copyRecord), stagingMemory(stagingMemory), dstImage(dstImage), oldLayout(oldLayout),
		copyLayout(copyLayout), newLayout(newLayout), newLayoutBarrierDstStages(newLayoutBarrierDstStages),
		newLayoutBarrierDstAccessFlags(newLayoutBarrierDstAccessFlags),
		regionCount(1), region(region), regionList(nullptr), dataSize(dataSize)  { stagingMemory->ref(); }
inline ImageMemory::BufferToImageUpload::BufferToImageUpload(StagingMemory* stagingMemory, CopyRecord* copyRecord, vk::Image dstImage,
	vk::ImageLayout oldLayout, vk::ImageLayout copyLayout, vk::ImageLayout newLayout,
	vk::PipelineStageFlags newLayoutBarrierDstStages, vk::AccessFlags newLayoutBarrierDstAccessFlags,
	uint32_t regionCount, std::unique_ptr<vk::BufferImageCopy[]>&& regionList, size_t dataSize) :
		copyRecord(copyRecord), stagingMemory(stagingMemory), dstImage(dstImage), oldLayout(oldLayout),
		copyLayout(copyLayout), newLayout(newLayout), newLayoutBarrierDstStages(newLayoutBarrierDstStages),
		newLayoutBarrierDstAccessFlags(newLayoutBarrierDstAccessFlags), regionCount(regionCount),
		regionList(regionList.release()), dataSize(dataSize)  { stagingMemory->ref(); }
inline ImageMemory::BufferToImageUpload::~BufferToImageUpload()  { delete[] regionList; stagingMemory->unref(); }

inline ImageMemory::ImageMemory(ImageStorage& imageStorage) : _imageStorage(&imageStorage)  {}
inline ImageStorage& ImageMemory::imageStorage() const  { return *_imageStorage; }
inline size_t ImageMemory::size() const  { return _bufferEndAddress; }
inline vk::DeviceMemory ImageMemory::memory() const  { return _memory; }
inline size_t ImageMemory::usedBytes() const  { return _usedBytes; }
inline bool ImageMemory::alloc(ImageAllocation& a, size_t numBytes, size_t alignment, vk::Image image, const vk::ImageCreateInfo& imageCreateInfo)  { if(a._record->size != 0) free(a._record); if(!allocInternal(a._record, numBytes, alignment)) return false; a._record->image=image; a._record->imageCreateInfo=imageCreateInfo; return true; }
inline void ImageMemory::freeInternal(ImageAllocationRecord*& recPtr) noexcept  { CircularAllocationMemory::freeInternal(recPtr); }

}
#endif
