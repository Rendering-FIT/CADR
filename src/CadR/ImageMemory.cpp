#include <CadR/ImageMemory.h>
#include <CadR/Renderer.h>

using namespace std;
using namespace CadR;



ImageMemory::~ImageMemory()
{
#if 0
	// release blocked memory markers
	// (they are used to block memory after it is scheduled for transfer and until the transfer is completed)
	if(_firstNotTransferredMarker1)
		releaseMemoryMarker1Chain(_firstNotTransferredMarker1);
	if(_firstNotTransferredMarker2)
		releaseMemoryMarker2Chain(_firstNotTransferredMarker2);
#endif

	// release DeviceMemory
	if(_memory) {
		VulkanDevice& device = _imageStorage->renderer().device();
		device.freeMemory(_memory);
	}
}


ImageMemory::ImageMemory(ImageStorage& imageStorage, size_t size, uint32_t memoryTypeIndex)
	: ImageMemory(imageStorage)  // this ensures the destructor will be executed if this constructor throws
{
	// initialize pointers
	_block1EndAddress = 0;
	_block1StartAddress = 0;
	_block2EndAddress = 0;
	_block2StartAddress = 0;
	_bufferEndAddress = size;
	_bufferStartAddress = 0;

	// handle zero-sized buffer
	if(size == 0)
		return;

	// allocate _memory
	Renderer& renderer = imageStorage.renderer();
	_memoryTypeIndex = memoryTypeIndex;
	_memory = renderer.allocateMemoryType(size, memoryTypeIndex);
}


ImageMemory::ImageMemory(ImageStorage& imageStorage, vk::DeviceMemory memory, size_t size, uint32_t memoryTypeIndex)
	: ImageMemory(imageStorage)  // this ensures the destructor will be executed if this constructor throws
{
	assert(((memory && size) || (!memory && size==0)) &&
	       "ImageMemory::ImageMemory(): The parameters memory and size must all be either non-zero/non-null or zero/null.");

	// assign resources
	// (do not throw in the code above before these are assigned to avoid leaked handles)
	_memory = memory;
	_memoryTypeIndex = memoryTypeIndex;

	// initialize pointers
	_block1EndAddress = 0;
	_block1StartAddress = 0;
	_block2EndAddress = 0;
	_block2StartAddress = 0;
	_bufferEndAddress = size;
	_bufferStartAddress = 0;
}


ImageMemory::ImageMemory(ImageStorage& imageStorage, nullptr_t)
	: _imageStorage(&imageStorage)
{
	_block1EndAddress = 0;
	_block1StartAddress = 0;
	_block2EndAddress = 0;
	_block2StartAddress = 0;
	_bufferEndAddress = 0;
	_bufferStartAddress = 0;
}


ImageMemory* ImageMemory::tryCreate(ImageStorage& imageStorage, size_t size, uint32_t memoryTypeIndex)
{
	Renderer& renderer = imageStorage.renderer();
	VulkanDevice& device = renderer.device();
	vk::Device d = device.get();

	// allocate memory
	vk::DeviceMemory m =
		renderer.allocateMemoryType(size, memoryTypeIndex);
	if(!m)
		return nullptr;

	// create DataMemory
	// (if it throws, it correctly releases b and m)
	try {
		return new ImageMemory(imageStorage, m, size, memoryTypeIndex);
	}
	catch(bad_alloc&) {
		d.freeMemory(m, nullptr, device);
		throw;
	}
}


bool ImageMemory::allocInternal(ImageAllocationRecord*& recPtr, size_t numBytes, size_t alignment)
{
	// propose allocation
	// (it will be confirmed when we call alloc[1|2]Commit(),
	// before commit no resources are really allocated)
	auto [offset, blockNumber] = allocPropose(numBytes, alignment);

	ImageAllocationRecord* rec;
	if(blockNumber == 1)
		rec = alloc1Commit(offset, numBytes);  // might throw
	else if(blockNumber == 2)
		rec = alloc2Commit(offset, numBytes);  // might throw
	else
		return false;

	// initialize allocation
	rec->init(offset, numBytes, this, &recPtr, nullptr);
	recPtr = rec;
	return true;
}


void ImageMemory::BufferToImageUpload::record(VulkanDevice& device, vk::CommandBuffer commandBuffer)
{
	if(!srcBuffer) return;

	if(regionCount <= 1) {

		// change image layout (oldLayout -> copyLayout)
		if(oldLayout != copyLayout)
			device.cmdPipelineBarrier(
				commandBuffer,  // commandBuffer
				vk::PipelineStageFlagBits::eTopOfPipe,  // srcStage
				vk::PipelineStageFlagBits::eTransfer,  // dstStage
				vk::DependencyFlags(),  // dependencyFlags
				0,  // memoryBarrierCount
				nullptr,  // pMemoryBarriers
				0,  // bufferMemoryBarrierCount
				nullptr,  // pBufferMemoryBarriers
				1,  // imageMemoryBarrierCount
				&(const vk::ImageMemoryBarrier&)vk::ImageMemoryBarrier(  // pImageMemoryBarriers
					vk::AccessFlags(),  // srcAccessMask
					vk::AccessFlagBits::eTransferWrite,  // dstAccessMask
					oldLayout,  // oldLayout
					copyLayout,  // newLayout
					VK_QUEUE_FAMILY_IGNORED,  // srcQueueFamilyIndex
					VK_QUEUE_FAMILY_IGNORED,  // dstQueueFamilyIndex
					dstImage,  // image
					vk::ImageSubresourceRange{  // subresourceRange
						region.imageSubresource.aspectMask,  // aspectMask
						region.imageSubresource.mipLevel,  // baseMipLevel
						1,  // levelCount
						region.imageSubresource.baseArrayLayer,  // baseArrayLayer
						region.imageSubresource.layerCount,  // layerCount
					}
				)
			);

		// transfer
		device.cmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, copyLayout, regionCount, &region);

		// change image layout (copyLayout -> newLayout)
		if(newLayoutBarrierStages != vk::PipelineStageFlags() || copyLayout != newLayout)
			device.cmdPipelineBarrier(
				commandBuffer,  // commandBuffer
				vk::PipelineStageFlagBits::eTransfer,  // srcStage
				newLayoutBarrierStages,  // dstStage
				vk::DependencyFlags(),  // dependencyFlags
				0,  // memoryBarrierCount
				nullptr,  // pMemoryBarriers
				0,  // bufferMemoryBarrierCount
				nullptr,  // pBufferMemoryBarriers
				1,  // imageMemoryBarrierCount
				&(const vk::ImageMemoryBarrier&)vk::ImageMemoryBarrier(  // pImageMemoryBarriers
					vk::AccessFlagBits::eTransferWrite,  // srcAccessMask
					newLayoutBarrierAccessFlags,  // dstAccessMask
					copyLayout,  // oldLayout
					newLayout,  // newLayout
					VK_QUEUE_FAMILY_IGNORED,  // srcQueueFamilyIndex
					VK_QUEUE_FAMILY_IGNORED,  // dstQueueFamilyIndex
					dstImage,  // image
					vk::ImageSubresourceRange{  // subresourceRange
						region.imageSubresource.aspectMask,  // aspectMask
						region.imageSubresource.mipLevel,  // baseMipLevel
						1,  // levelCount
						region.imageSubresource.baseArrayLayer,  // baseArrayLayer
						region.imageSubresource.layerCount,  // layerCount
					}
				)
			);

	}
	else {

		unique_ptr<vk::ImageMemoryBarrier[]> imageMemoryBarrierList = make_unique<vk::ImageMemoryBarrier[]>(regionCount);

		if(oldLayout != copyLayout) {

			imageMemoryBarrierList = make_unique<vk::ImageMemoryBarrier[]>(regionCount);
			for(size_t i=0; i<regionCount; i++) {
				imageMemoryBarrierList[i] = {
					vk::AccessFlags(),  // srcAccessMask
					vk::AccessFlagBits::eTransferWrite,  // dstAccessMask
					oldLayout,  // oldLayout
					copyLayout,  // newLayout
					VK_QUEUE_FAMILY_IGNORED,  // srcQueueFamilyIndex
					VK_QUEUE_FAMILY_IGNORED,  // dstQueueFamilyIndex
					dstImage,  // image
					vk::ImageSubresourceRange{  // subresourceRange
						regionList[i].imageSubresource.aspectMask,  // aspectMask
						regionList[i].imageSubresource.mipLevel,  // baseMipLevel
						1,  // levelCount
						regionList[i].imageSubresource.baseArrayLayer,  // baseArrayLayer
						regionList[i].imageSubresource.layerCount,  // layerCount
					}
				};
			}

			// change image layout (oldLayout -> copyLayout)
			device.cmdPipelineBarrier(
				commandBuffer,  // commandBuffer
				vk::PipelineStageFlagBits::eTopOfPipe,  // srcStage
				vk::PipelineStageFlagBits::eTransfer,  // dstStage
				vk::DependencyFlags(),  // dependencyFlags
				0,  // memoryBarrierCount
				nullptr,  // pMemoryBarriers
				0,  // bufferMemoryBarrierCount
				nullptr,  // pBufferMemoryBarriers
				regionCount,  // imageMemoryBarrierCount
				imageMemoryBarrierList.get()  // pImageMemoryBarriers
			);

		}

		// transfer
		device.cmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, copyLayout, regionCount, regionList);

		// change image layout (copyLayout -> newLayout)
		if(newLayoutBarrierStages != vk::PipelineStageFlags() || copyLayout != newLayout)

			if(!imageMemoryBarrierList) {

				imageMemoryBarrierList = make_unique<vk::ImageMemoryBarrier[]>(regionCount);
				for(size_t i=0; i<regionCount; i++) {
					imageMemoryBarrierList[i] = {
						vk::AccessFlagBits::eTransferWrite,  // srcAccessMask
						newLayoutBarrierAccessFlags,  // dstAccessMask
						copyLayout,  // oldLayout
						newLayout,  // newLayout
						VK_QUEUE_FAMILY_IGNORED,  // srcQueueFamilyIndex
						VK_QUEUE_FAMILY_IGNORED,  // dstQueueFamilyIndex
						dstImage,  // image
						vk::ImageSubresourceRange{  // subresourceRange
							regionList[i].imageSubresource.aspectMask,  // aspectMask
							regionList[i].imageSubresource.mipLevel,  // baseMipLevel
							1,  // levelCount
							regionList[i].imageSubresource.baseArrayLayer,  // baseArrayLayer
							regionList[i].imageSubresource.layerCount,  // layerCount
						}
					};
				}

			}
			else {

				for(size_t i=0; i<regionCount; i++) {
					vk::ImageMemoryBarrier& imb = imageMemoryBarrierList[i];
					imb.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
					imb.dstAccessMask = newLayoutBarrierAccessFlags;
					imb.oldLayout = copyLayout;
					imb.newLayout = newLayout;
				}

			}

			device.cmdPipelineBarrier(
				commandBuffer,  // commandBuffer
				vk::PipelineStageFlagBits::eTransfer,  // srcStage
				newLayoutBarrierStages,  // dstStage
				vk::DependencyFlags(),  // dependencyFlags
				0,  // memoryBarrierCount
				nullptr,  // pMemoryBarriers
				0,  // bufferMemoryBarrierCount
				nullptr,  // pBufferMemoryBarriers
				regionCount,  // imageMemoryBarrierCount
				imageMemoryBarrierList.get()  // pImageMemoryBarriers
			);

	}

}


ImageAllocationBlock::ImageAllocationBlock(CircularAllocationMemory<ImageAllocationRecord, ImageRecordsPerBlock,
	ImageAllocationBlock>& circularAllocationMemory)
{
	VulkanDevice& d = static_cast<ImageMemory&>(circularAllocationMemory).imageStorage().renderer().device();
	descriptorPool =
		d.createDescriptorPool(
			vk::DescriptorPoolCreateInfo(
				vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
				1,
				1,
				&vk::DescriptorPoolSize{
					vk::DescriptorType::eCombinedImageSampler,
					ImageRecordsPerBlock,
				}
			)
		);

	for(size_t i=1,c=ImageRecordsPerBlock; i<=c; i++)
		allocations[i].descriptorPool = descriptorPool;
}
