#include <CadR/ImageAllocation.h>

using namespace std;
using namespace CadR;


ImageAllocationRecord ImageAllocationRecord::nullRecord{ 0, 0, nullptr, nullptr, nullptr, nullptr };



void ImageAllocationRecord::releaseHandles(VulkanDevice& device) noexcept
{
	// no CopyRecord attached
	if(!copyRecord) {
		device.destroy(image);
		image = nullptr;
		return;
	}

	// image handle
	if(copyRecord->copyOpCounter==0) {
		device.destroy(image);
		image = nullptr;
	}
	else
		copyRecord->imageToDestroy = image;

	// detach CopyRecord
	// (it will be destroyed when copyOpCounter or referenceCounter is decremented
	// and both become zero)
	copyRecord->imageAllocationRecord = nullptr;
	copyRecord = nullptr;
}


#if 0
void ImageAllocation::upload(const void* ptr, size_t numBytes)
{
	_record = imageStorage().realloc(_record, numBytes);
	memcpy(_record->stagingData, ptr, numBytes);
}
#endif
