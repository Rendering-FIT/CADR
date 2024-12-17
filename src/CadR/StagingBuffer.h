#ifndef CADR_STAGING_BUFFER_HEADER
# define CADR_STAGING_BUFFER_HEADER

# include <cstdint>
# include <vulkan/vulkan.hpp>

namespace CadR {

class DataAllocation;
class HandlelessAllocation;
class ImageAllocation;
class ImageStorage;
class StagingMemory;


class CADR_EXPORT StagingBuffer {
protected:
	uint64_t _startAddress;  //< Host address of the first byte of the staging buffer as seen from the CPU.
	uint64_t _size;  //< Size in bytes of the staging buffer.
	StagingMemory* _stagingMemory;  //< StagingMemory from which this StagingBuffer is allocated.
public:

	// constructors and destructors
	//StagingBuffer(DataStorage& dataStorage, size_t numBytes, size_t alignment);
	StagingBuffer(ImageStorage& imageStorage, size_t numBytes, size_t alignment);
	StagingBuffer(StagingMemory& stagingMemory, void* startAddress, size_t numBytes) noexcept;
	~StagingBuffer() noexcept;

	// copy and move constructors and operators
	StagingBuffer(StagingBuffer&& other) noexcept;
	StagingBuffer(const StagingBuffer&) = delete;
	StagingBuffer& operator=(StagingBuffer&& rhs) noexcept;
	StagingBuffer& operator=(const StagingBuffer&) = delete;

	void submit(DataAllocation& a);
	void submit(HandlelessAllocation& a);
	void submit(ImageAllocation& a, vk::ImageLayout currentLayout, vk::ImageLayout copyLayout,
	            vk::ImageLayout newLayout, vk::PipelineStageFlags newLayoutBarrierStageFlags,
	            vk::AccessFlags newLayoutBarrierAccessFlags, const vk::BufferImageCopy& region);
	void submit(ImageAllocation& a, vk::ImageLayout currentLayout, vk::ImageLayout copyLayout,
	            vk::ImageLayout newLayout, vk::PipelineStageFlags newLayoutBarrierStageFlags,
	            vk::AccessFlags newLayoutBarrierAccessFlags, vk::Extent2D imageExtent);

	template<typename T> T* data();
	template<typename T> T* data(size_t offset);
	size_t sizeInBytes() const;
	StagingMemory* stagingMemory() const;
	vk::Buffer buffer() const;
	uint64_t bufferOffset() const;

};


}

#endif


// inline functions
#if !defined(CADR_STAGING_BUFFER_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_STAGING_BUFFER_INLINE_FUNCTIONS
# define CADR_NO_INLINE_FUNCTIONS
# include <CadR/ImageStorage.h>
# include <CadR/StagingMemory.h>
# undef CADR_NO_INLINE_FUNCTIONS
namespace CadR {

//inline StagingBuffer::StagingBuffer(DataStorage& dataStorage, size_t numBytes, size_t alignment)  : StagingBuffer(std::move(dataStorage.createStagingBuffer(numBytes, alignment)))  {}
inline StagingBuffer::StagingBuffer(ImageStorage& imageStorage, size_t numBytes, size_t alignment)  : StagingBuffer(std::move(imageStorage.createStagingBuffer(numBytes, alignment)))  {}
inline StagingBuffer::StagingBuffer(StagingBuffer&& other) noexcept  : _startAddress(other._startAddress), _size(other._size), _stagingMemory(other._stagingMemory) { other._stagingMemory=nullptr; }
inline StagingBuffer& StagingBuffer::operator=(StagingBuffer&& rhs) noexcept  { if(_stagingMemory && _stagingMemory != rhs._stagingMemory) _stagingMemory->unref(); _startAddress=rhs._startAddress; _size=rhs._size; _stagingMemory=rhs._stagingMemory; rhs._stagingMemory=nullptr; return *this; }
template<typename T> inline T* StagingBuffer::data()  { return reinterpret_cast<T*>(_startAddress); }
template<typename T> inline T* StagingBuffer::data(size_t offset)  { return reinterpret_cast<T*>(_startAddress+offset); }
inline size_t StagingBuffer::sizeInBytes() const  { return _size; }
inline StagingMemory* StagingBuffer::stagingMemory() const  { return _stagingMemory; }
inline vk::Buffer StagingBuffer::buffer() const  { return _stagingMemory->buffer(); }
inline uint64_t StagingBuffer::bufferOffset() const  { return _startAddress - _stagingMemory->_bufferStartAddress; }

}
#endif
