#pragma once

#include <vulkan/vulkan.hpp>

namespace CadR {

class Renderer;


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
	StagingBuffer(vk::Buffer dstBuffer,size_t dstOffset,size_t size,Renderer* renderer);
	~StagingBuffer();

	StagingBuffer(const StagingBuffer&) = delete;
	StagingBuffer(StagingBuffer&& other) noexcept;
	StagingBuffer& operator=(const StagingBuffer&) = delete;
	StagingBuffer& operator=(StagingBuffer&& rhs) noexcept;

	uint8_t* data() const;
	size_t size() const;

	void submit();
	void scheduleCopy(vk::CommandBuffer cb);
	void reset(vk::Buffer dstBuffer,size_t dstOffset,size_t size,Renderer* renderer);
	void destroy();

protected:
	void init();
	void cleanUp();
};


// inline methods
inline StagingBuffer::StagingBuffer(Renderer* renderer)
	: _renderer(renderer), _data(nullptr), _size(0)  {}
inline StagingBuffer::StagingBuffer(vk::Buffer dstBuffer,size_t dstOffset,size_t size,Renderer* renderer)
	: _renderer(renderer), _size(size), _dstBuffer(dstBuffer), _dstOffset(dstOffset)  { init(); }
inline StagingBuffer::~StagingBuffer()  { cleanUp(); }

inline uint8_t* StagingBuffer::data() const  { return _data; }
inline size_t StagingBuffer::size() const  { return _size; }

inline void StagingBuffer::destroy()  { cleanUp(); }

}
