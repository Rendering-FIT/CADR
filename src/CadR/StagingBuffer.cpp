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
                           vk::PipelineStageFlags newLayoutBarrierStageFlags, vk::AccessFlags newLayoutBarrierAccessFlags,
                           const vk::BufferImageCopy& region)
{
	// skip invalid and already freed allocations
	ImageAllocationRecord* r = a._record;
	if(r->size == 0)
		return;

	// CopyRecord
	if(r->copyRecord == nullptr) {
		r->copyRecord = new CopyRecord;
		r->copyRecord->imageAllocation = &a;
		r->copyRecord->referenceCounter = 0;
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
			newLayoutBarrierStageFlags,
			newLayoutBarrierAccessFlags,
			region);
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
                           vk::PipelineStageFlags newLayoutBarrierStageFlags, vk::AccessFlags newLayoutBarrierAccessFlags,
                           vk::Extent2D imageExtent)
{
	submit(a, oldLayout, copyLayout, newLayout,
	       newLayoutBarrierStageFlags, newLayoutBarrierAccessFlags,
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
	       });
}
