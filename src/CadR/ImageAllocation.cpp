#include <CadR/ImageAllocation.h>

using namespace std;
using namespace CadR;


ImageAllocationRecord ImageAllocationRecord::nullRecord{ 0, 0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };



#if 0
void ImageAllocation::upload(const void* ptr, size_t numBytes)
{
	_record = imageStorage().realloc(_record, numBytes);
	memcpy(_record->stagingData, ptr, numBytes);
}
#endif
