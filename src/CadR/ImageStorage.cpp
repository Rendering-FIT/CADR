#include <CadR/ImageStorage.h>
#include <CadR/Exceptions.h>
#include <CadR/Renderer.h>
#include <CadR/StagingManager.h>
#include <CadR/StagingMemory.h>
#include <CadR/TransferResources.h>

using namespace std;
using namespace CadR;



void ImageStorage::cleanUp() noexcept
{
	//assert(_transferInProgressList.empty() && "ImageStorage::cleanUp(): All transfers must be finished before calling cleanUp() function.");

	// destroy all handles
	//_handleTable.destroyAll();

	// destroy ImageMemory objects
	for(MemoryTypeManagement& mtm : _memoryTypeManagementList)
		for(ImageMemory* im : mtm._imageMemoryList)
			delete im;
	_memoryTypeManagementList.clear();
}


bool ImageStorage::allocInternalFromMemoryType(ImageAllocationRecord*& recPtr, size_t numBytes, size_t alignment, uint32_t memoryTypeIndex)
{
	MemoryTypeManagement& mtm = _memoryTypeManagementList[memoryTypeIndex];

	// make sure we have _firstAllocMemory
	// (it might be missing during the first call to alloc())
	if(mtm._firstAllocMemory == nullptr) {
		size_t size =
			(numBytes < Renderer::mediumMemorySize)
				? Renderer::mediumMemorySize
				: max(numBytes, Renderer::largeMemorySize);
		mtm._firstAllocMemory = ImageMemory::tryCreate(*this, size, memoryTypeIndex);
		if(mtm._firstAllocMemory == nullptr)
			return false;
		mtm._imageMemoryList.emplace_back(mtm._firstAllocMemory);
	}

	// try alloc from _firstAllocMemory
	if(mtm._firstAllocMemory->allocInternal(recPtr, numBytes, alignment))
		return true;

	// make sure we have _secondAllocMemory
	// (it might be missing until the first DataMemory is full)
	if(mtm._secondAllocMemory == nullptr) {
		size_t size =
			max(numBytes, Renderer::largeMemorySize);
		mtm._secondAllocMemory = ImageMemory::tryCreate(*this, size, memoryTypeIndex);
		if(mtm._secondAllocMemory == nullptr)
			return false;
		mtm._imageMemoryList.emplace_back(mtm._secondAllocMemory);
	}

	// the alloc from _secondAllocMemory
	if(mtm._secondAllocMemory->allocInternal(recPtr, numBytes, alignment))
		return true;

	// create new DataMemory
	// and set is as _secondAllocMemory
	// (_firstAllocMemory is considered full now, _secondAllocMemory almost full,
	// so we replace _firstAlloc memory by _secondAllocMemory and
	// we put new DataMemory into _secondAllocMemory)
	size_t size = max(Renderer::largeMemorySize, numBytes);
	ImageMemory* m = ImageMemory::tryCreate(*this, size, memoryTypeIndex);
	if(m == nullptr)
		return false;
	mtm._imageMemoryList.emplace_back(m);
	mtm._firstAllocMemory = mtm._secondAllocMemory;
	mtm._secondAllocMemory = m;

	// make the allocation
	// from the new DataMemory
	if(mtm._secondAllocMemory->allocInternal(recPtr, numBytes, alignment))
		return true;

	// new ImageMemory was created and we cannot alloc? => something strange, but could theoretically happen
	throw OutOfResources("CadR::ImageStorage::allocFromMemoryType() error: Cannot allocate ImageAllocation "
	                     "although new ImageMemory was created successfully. "
	                     "Requested size: " + to_string(size) + " bytes.");
}


void ImageStorage::allocInternal(ImageAllocationRecord*& recPtr, size_t numBytes, size_t alignment,
	uint32_t memoryTypeBits, vk::MemoryPropertyFlags requiredFlags)
{
	// forward the call to appropriate memory type
	const vk::PhysicalDeviceMemoryProperties& memoryProperties = _renderer->memoryProperties();
	for(uint32_t i=0, c=memoryProperties.memoryTypeCount; i<c; i++)
		if(memoryTypeBits & (1<<i))
			if((memoryProperties.memoryTypes[i].propertyFlags & requiredFlags) == requiredFlags)
				if(allocInternalFromMemoryType(recPtr, numBytes, alignment, i))
					return;
	throw OutOfResources("CadR::ImageStorage::allocFromMemoryType() error: Cannot allocate ImageMemory. "
	                     "Requested size: " + to_string(numBytes) + " bytes.");
}


void ImageStorage::alloc(ImageAllocation& a, size_t numBytes, size_t alignment,
	uint32_t memoryTypeBits, vk::MemoryPropertyFlags requiredFlags,
	vk::Image image, const vk::ImageCreateInfo& imageCreateInfo)
{
	if(a._record->size != 0)
		free(a);

	allocInternal(a._record, numBytes, alignment, memoryTypeBits, requiredFlags);
	_renderer->device().bindImageMemory(image, a._record->imageMemory->memory(), a._record->memoryOffset);
	a._record->image = image;
	a._record->imageCreateInfo = imageCreateInfo;
	a._record->callImageChangedCallbacks();
}


StagingBuffer ImageStorage::createStagingBuffer(size_t numBytes, size_t alignment)
{
	// try to reuse recently allocated StagingMemory
	// if there is enough unused space in it
	if(_lastStagingMemory) {
		size_t smSize = _lastStagingMemory->size();
		if(smSize - _lastStagingMemory->_numBytesAllocated >= numBytes + alignment) {
			size_t offset = (_lastStagingMemory->_numBytesAllocated - 1) / alignment * (alignment + 1);
			size_t newNumBytesAllocated = offset + numBytes;
			if(newNumBytesAllocated <= smSize) {
				_lastStagingMemory->_numBytesAllocated = newNumBytesAllocated;
				return StagingBuffer(*_lastStagingMemory, _lastStagingMemory->data(offset), numBytes);  // does not throw
			}
		}
	}

	// create small StagingMemory
	StagingMemory* sm;
	if(_lastFrameStagingBytesTransferred <= Renderer::smallMemorySize &&
	   _currentFrameSmallMemoryCount == 0 && numBytes <= Renderer::smallMemorySize)
	{
		_currentFrameSmallMemoryCount = 1;
		sm = &_stagingManager->reuseOrAllocSmallStagingMemory();
		goto haveMemory;
	}

	// create medium StagingMemory
	if(_lastFrameStagingBytesTransferred <= Renderer::mediumMemorySize &&
	   _currentFrameMediumMemoryCount == 0 && numBytes <= Renderer::mediumMemorySize)
	{
		_currentFrameMediumMemoryCount = 1;
		sm = &_stagingManager->reuseOrAllocMediumStagingMemory();
		goto haveMemory;
	}

	// create large StagingMemory
	if(numBytes <= Renderer::largeMemorySize)
	{
		_currentFrameLargeMemoryCount++;
		sm = &_stagingManager->reuseOrAllocLargeStagingMemory();
		goto haveMemory;
	}

	// create super-size StagingMemory
	_currentFrameSuperSizeMemoryCount++;
	sm = &_stagingManager->reuseOrAllocSuperSizeStagingMemory(numBytes);

haveMemory:
	sm->_numBytesAllocated = numBytes;
	sm->_referenceCounter = 0;
	return StagingBuffer(*sm, sm->data(), numBytes);  // does not throw
}


void ImageStorage::endFrame()
{
	_lastFrameStagingBytesTransferred = _currentFrameStagingBytesTransferred;
	_currentFrameStagingBytesTransferred = 0;
	_currentFrameSmallMemoryCount = 0;
	_currentFrameMediumMemoryCount = 0;
	_currentFrameLargeMemoryCount = 0;
	_currentFrameSuperSizeMemoryCount = 0;
}


tuple<TransferResources,size_t> ImageStorage::recordUploads(vk::CommandBuffer commandBuffer)
{
	vector<tuple<ImageMemory*,void*>> transferResourceList;
	size_t totalDataSize = 0;
	void* transferResource;
	size_t dataSize;
	for(MemoryTypeManagement& mtm : _memoryTypeManagementList)
		for(ImageMemory* im : mtm._imageMemoryList) {
			tie(transferResource, dataSize) =
				im->recordUploads(commandBuffer);
			if(transferResource != nullptr) {
				transferResourceList.emplace_back(im, transferResource);
				totalDataSize += dataSize;
			}
		}

	// return
	if(transferResourceList.empty())
		return {TransferResources(), totalDataSize};
	else
		return {
			TransferResources(
				[](vector<tuple<ImageMemory*,void*>>& transferResourceList) {
					for(auto& item : transferResourceList)
						get<0>(item)->uploadDone(get<1>(item));
				},
				move(transferResourceList)
			),
			totalDataSize
		};
}
