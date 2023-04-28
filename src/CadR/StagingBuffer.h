#pragma once

#include <vulkan/vulkan.hpp>

namespace CadR {

class Renderer;
class StagingManager;


class CADR_EXPORT StagingBuffer final {
protected:

	Renderer* _renderer;  ///< Renderer associated with this StagingBuffer.
	vk::Buffer _stgBuffer;  ///< Staging buffer handle.
	vk::DeviceMemory _stgMemory;  ///< Staging memory handle.
	uint8_t* _data;  ///< Pointer to the mapped staging memory.
	size_t _size;  ///< Size of staging buffer and staging memory.
	vk::Buffer _dstBuffer;  ///< Destination buffer that will be updated by the staging buffer.
	size_t _dstOffset;  ///< Offset into the destination buffer. Staging data will overwrite in destination buffer starting from this offset.

	friend Renderer;

public:

	StagingBuffer() = delete;
	StagingBuffer(Renderer* renderer);
	StagingBuffer(Renderer* renderer,vk::Buffer dstBuffer,size_t dstOffset,size_t size);
	~StagingBuffer();
	void destroy();

	StagingBuffer(const StagingBuffer&) = delete;
	StagingBuffer(StagingBuffer&& other) noexcept;
	StagingBuffer& operator=(const StagingBuffer&) = delete;
	StagingBuffer& operator=(StagingBuffer&& rhs) noexcept;

	template<typename T = uint8_t> T* data() const;
	template<typename T = uint8_t> size_t size() const;

	void reset(Renderer* renderer,vk::Buffer dstBuffer,size_t dstOffset,size_t size);
	void record(vk::CommandBuffer cb);

protected:
	void init();
	void cleanUp();
	friend StagingManager;
};


// inline methods
inline StagingBuffer::StagingBuffer(Renderer* renderer)
	: _renderer(renderer), _data(nullptr), _size(0)  {}
inline StagingBuffer::StagingBuffer(Renderer* renderer,vk::Buffer dstBuffer,size_t dstOffset,size_t size)
	: _renderer(renderer), _size(size), _dstBuffer(dstBuffer), _dstOffset(dstOffset)  { init(); }
inline StagingBuffer::~StagingBuffer()  { cleanUp(); }
inline void StagingBuffer::destroy()  { cleanUp(); }

template<typename T> inline T* StagingBuffer::data() const  { return reinterpret_cast<T*>(_data); }
template<typename T> inline size_t StagingBuffer::size() const  { return _size / sizeof(T); }


}
