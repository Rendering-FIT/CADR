#include <CadR/StagingBuffer.h>
#include <CadR/ImageAllocation.h>
#include <CadR/ImageMemory.h>
#include <CadR/StagingMemory.h>

using namespace CadR;



StagingBuffer::~StagingBuffer() noexcept
{
	if(_stagingMemory)
		_stagingMemory->unref();
}


StagingBuffer::StagingBuffer(StagingMemory& stagingMemory, void* startAddress, size_t numBytes) noexcept
	: _startAddress(uint64_t(startAddress))
	, _size(numBytes)
	, _stagingMemory(&stagingMemory)
{
	stagingMemory.ref();
}


void StagingBuffer::submit(DataAllocation& a)
{
}


void StagingBuffer::submit(ImageAllocation& a,
                           vk::ImageLayout oldLayout, vk::ImageLayout copyLayout, vk::ImageLayout newLayout,
                           vk::PipelineStageFlags newLayoutBarrierDstStages, vk::AccessFlags newLayoutBarrierDstAccessFlags,
                           const vk::BufferImageCopy& region, size_t dataSize)
{
	// skip invalid and already freed allocations
	ImageAllocationRecord* r = a._record;
	if(r->size == 0)
		return;

	// CopyRecord
	if(r->copyRecord == nullptr) {
		r->copyRecord = new CopyRecord;
		r->copyRecord->imageAllocationRecord = r;
		r->copyRecord->referenceCounter = 0;
		r->copyRecord->copyOpCounter = 0;
	}

	// append record to bufferToImageUploadList
	ImageMemory* m = r->imageMemory;
	try {
		m->_bufferToImageUploadList.emplace_back(  // might theoretically throw
			r->copyRecord,
			_stagingMemory->buffer(),
			r->image,
			oldLayout,
			copyLayout,
			newLayout,
			newLayoutBarrierDstStages,
			newLayoutBarrierDstAccessFlags,
			region,
			dataSize);
	}
	catch(...) {
		// delete CopyRecord if we just created it
		if(r->copyRecord->referenceCounter == 0) {
			delete r->copyRecord;
			r->copyRecord = nullptr;
		}
		throw;
	}
	r->copyRecord->referenceCounter++;
}


void StagingBuffer::submit(ImageAllocation& a,
                           vk::ImageLayout oldLayout, vk::ImageLayout copyLayout, vk::ImageLayout newLayout,
                           vk::PipelineStageFlags newLayoutBarrierDstStages, vk::AccessFlags newLayoutBarrierDstAccessFlags,
                           vk::Extent2D imageExtent, size_t dataSize)
{
	submit(a, oldLayout, copyLayout, newLayout,
	       newLayoutBarrierDstStages, newLayoutBarrierDstAccessFlags,
	       vk::BufferImageCopy{
		       bufferOffset(),  // bufferOffset
		       imageExtent.width,  // bufferRowLength
		       imageExtent.height,  // bufferImageHeight
		       vk::ImageSubresourceLayers{  // imageSubresource
			       vk::ImageAspectFlagBits::eColor,  // aspectMask
			       0,  // mipLevel
			       0,  // baseArrayLayer
			       1,  // layerCount
		       },
		       {0, 0, 0},  // imageOffset
		       {imageExtent.width, imageExtent.height, 1}  // imageExtent
	       },
	       dataSize);
}
