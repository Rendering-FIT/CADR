#pragma once

#include <vulkan/vulkan.hpp>
#include <CadR/Export.h>

namespace CadR {

class Renderer;


class CADR_EXPORT StagingBuffer final {
protected:

	Renderer* _renderer;
	vk::Buffer _stgBuffer;
	vk::DeviceMemory _stgMemory;
	uint8_t* _data;
	size_t _size;
	vk::Buffer _dstBuffer;
	size_t _dstOffset;

	friend Renderer;

public:

	StagingBuffer() = delete;
	StagingBuffer(vk::Buffer dstBuffer,size_t dstOffset,size_t size,Renderer* renderer);
	~StagingBuffer();

	StagingBuffer(const StagingBuffer&) = delete;
	StagingBuffer(StagingBuffer&& other) noexcept;
	StagingBuffer& operator=(const StagingBuffer&) = delete;
	StagingBuffer& operator=(StagingBuffer&& rhs) noexcept;

	uint8_t* data() const;
	size_t size() const;

	void submit();
	void reset(vk::Buffer dstBuffer,size_t dstOffset,size_t size,Renderer* renderer);

protected:
	void init();
	void cleanUp();
};


// inline methods
inline StagingBuffer::StagingBuffer(vk::Buffer dstBuffer,size_t dstOffset,size_t size,Renderer* renderer)
	: _renderer(renderer), _size(size), _dstBuffer(dstBuffer), _dstOffset(dstOffset)  { init(); }
inline StagingBuffer::~StagingBuffer()  { cleanUp(); }

inline uint8_t* StagingBuffer::data() const  { return _data; }
inline size_t StagingBuffer::size() const  { return _size; }


}
