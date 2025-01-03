#ifndef CADR_IMAGE_STORAGE_HEADER
# define CADR_IMAGE_STORAGE_HEADER

# ifndef CADR_NO_INLINE_FUNCTIONS
#  define CADR_NO_INLINE_FUNCTIONS
#  include <CadR/ImageMemory.h>
#  include <CadR/TransferResources.h>
#  undef CADR_NO_INLINE_FUNCTIONS
# else
#  include <CadR/ImageMemory.h>
#  include <CadR/TransferResources.h>
# endif
#include <vector>

namespace CadR {

class Renderer;
class StagingManager;
class VulkanDevice;


class CADR_EXPORT ImageStorage {
protected:

	Renderer* _renderer;  ///< Rendering context associated with the DataStorage.
	struct MemoryTypeManagement {
		ImageMemory* _firstAllocMemory;
		ImageMemory* _secondAllocMemory;
		std::vector<ImageMemory*> _imageMemoryList;
	};
	std::vector<MemoryTypeManagement> _memoryTypeManagementList;
	StagingManager* _stagingManager = nullptr;
	StagingMemory* _lastStagingMemory = nullptr;
	ImageMemory _zeroSizeImageMemory = ImageMemory(*this, nullptr);
	ImageAllocationRecord _zeroSizeAllocationRecord =
		ImageAllocationRecord{ 0, 0, &_zeroSizeImageMemory, nullptr, nullptr, nullptr };
	size_t _lastFrameStagingBytesTransferred = 0;
	size_t _currentFrameStagingBytesTransferred = 0;
	size_t _currentFrameSmallMemoryCount = 0;
	size_t _currentFrameMediumMemoryCount = 0;
	size_t _currentFrameLargeMemoryCount = 0;
	size_t _currentFrameSuperSizeMemoryCount = 0;

	bool allocFromMemoryType(ImageAllocation& a, size_t numBytes, size_t alignment, uint32_t memoryTypeIndex,
		vk::Image image, const vk::ImageCreateInfo& imageCreateInfo);
		//< Allocates memory for the image and Vulkan handles.
		//< If ImageAllocation already contains valid alocation, it is freed before the new allocation is attempted.
		//< It returns true in the case of success. False is returned if there is not enough free space and more space cannot be allocated.
		//< In the case of other errors, exceptions are thrown, such as bad_alloc or Vulkan exceptions.
	bool allocInternalFromMemoryType(ImageAllocationRecord*& recPtr, size_t numBytes, size_t alignment, uint32_t memoryTypeIndex);
		//< Allocates memory for the image but does not allocate handles.
		//< Memory is allocated from the memory type given by memoryTypIndex as returned by vkGetPhysicalDeviceMemoryProperties().
		//< The recPtr must point to ImageAllocation::_record variable that will be overwritten - e.g. it might be null or unitialized memory, etc.
		//< It should not contain valid ImageAllocationRecord pointer because the pointer will not be freed. Returns true in the case of success.
		//< It returns false if there is not enough free space. It might throw in the case of error, such as bad_alloc.
		//< If false is returned or exception is thrown, variable pointed by recPtr stays intact.

public:

	// construction and destruction
	ImageStorage(Renderer& r);
	~ImageStorage() noexcept;
	void init(StagingManager& stagingManager, uint32_t memoryTypeCount);
	void cleanUp() noexcept;

	// deleted constructors and operators
	ImageStorage(const ImageStorage&) = delete;
	ImageStorage(ImageStorage&&) = delete;
	ImageStorage& operator=(const ImageStorage&) = delete;
	ImageStorage& operator=(ImageStorage&&) = delete;

	// getters
	Renderer& renderer() const;
	ImageAllocationRecord* zeroSizeAllocationRecord() noexcept;

	// alloc-related functions
	void alloc(ImageAllocation& a, size_t numBytes, size_t alignment, uint32_t memoryTypeBits, vk::MemoryPropertyFlags requiredFlags,
		vk::Image image, const vk::ImageCreateInfo& imageCreateInfo);
		//< Allocates memory for the image and Vulkan handles.
		//< Memory is allocated from a memory type that is determined by requiredFlags and memoryTypeBits (as returned by vkGetPhysicalDeviceMemoryProperties()).
		//< If ImageAllocation already contains valid alocation, it is freed before the new allocation is attempted.
		//< If there is not enough memory or an error occured, an exception is thrown.
	void allocInternal(ImageAllocationRecord*& recPtr, size_t numBytes, size_t alignment, uint32_t memoryTypeBits, vk::MemoryPropertyFlags requiredFlags);
		//< Allocates memory for the image but does not allocate handles.
		//< Memory is allocated from a memory type that is determined by requiredFlags and memoryTypeBits (as returned by vkGetPhysicalDeviceMemoryProperties()).
		//< The recPtr must point to ImageAllocation::_record variable that will be overwritten - e.g. it might be null or unitialized memory, etc.
		//< It should not contain valid ImageAllocationRecord pointer because the pointer will not be freed.
		//< If there is not enough memory or an error occured, an exception is thrown and variable pointed by recPtr stays intact.
	void free(ImageAllocation& a) noexcept;
	StagingBuffer createStagingBuffer(size_t numBytes, size_t alignment);

	// upload functions
	std::tuple<TransferResources,size_t> recordUploads(vk::CommandBuffer commandBuffer);
	void endFrame();

};


}

#endif


// inline functions
#if !defined(CADR_IMAGE_STORAGE_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_IMAGE_STORAGE_INLINE_FUNCTIONS
namespace CadR {

inline bool ImageStorage::allocFromMemoryType(ImageAllocation& a, size_t numBytes, size_t alignment, uint32_t memoryTypeIndex, vk::Image image, const vk::ImageCreateInfo& imageCreateInfo)  { if(a._record->size != 0) free(a); if(!allocInternalFromMemoryType(a._record, numBytes, alignment, memoryTypeIndex)) return false; a._record->image=image; a._record->imageCreateInfo=imageCreateInfo; return true; }
inline ImageStorage::ImageStorage(Renderer& r) : _renderer(&r), _stagingManager(nullptr)  {}
inline ImageStorage::~ImageStorage() noexcept  { cleanUp(); }
inline void ImageStorage::init(StagingManager& stagingManager, uint32_t memoryTypeCount)  { _stagingManager = &stagingManager; _memoryTypeManagementList.resize(memoryTypeCount); }
inline Renderer& ImageStorage::renderer() const  { return *_renderer; }
inline ImageAllocationRecord* ImageStorage::zeroSizeAllocationRecord() noexcept  { return &_zeroSizeAllocationRecord; }
inline void ImageStorage::free(ImageAllocation& a) noexcept  { a.free(); }

}
#endif
