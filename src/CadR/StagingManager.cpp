#include <CadR/StagingManager.h>

using namespace std;
using namespace CadR;


StagingManager::StagingManager(bool subsetUpdateMode,uint32_t allocationID,StagingManagerList& l,unsigned numBuffers) noexcept
{
	if(subsetUpdateMode) {
		_state=0x1; // subsetUpdateMode=1, vectorAllocated=0
		_stagingBuffer._renderer=nullptr;
	}
	else
		if(numBuffers==1) {
			_state=0; // subsetUpdateMode=0, vectorAllocated=0
			_stagingBuffer._renderer=nullptr;
		}
		else {
			_state=0x2; // subsetUpdateMode=0, vectorAllocated=1
			new(&_stagingBufferList) vector<StagingBuffer>();
			_stagingBufferList.reserve(numBuffers);
			for(size_t i=0; i<numBuffers; i++)
				_stagingBufferList.emplace_back(nullptr);
		}
	_allocationID=allocationID;
	l.push_back(*this);
}


StagingBuffer& StagingManager::createStagingBuffer(Renderer* renderer,vk::Buffer dstBuffer,size_t dstOffset,size_t size)
{
	if(!subsetUpdateMode())
		if(!vectorAllocated()) {
			if(_stagingBuffer._renderer==nullptr) {
				new(&_stagingBuffer) StagingBuffer(renderer,dstBuffer,dstOffset,size);
				return _stagingBuffer;
			}
			else {
				_stagingBuffer.reset(renderer,dstBuffer,dstOffset,size);
				return _stagingBuffer;
			}
		}
		else {
			_stagingBufferList[0].reset(renderer,dstBuffer,dstOffset,size);
			return _stagingBufferList[0];
		}
	else
		if(vectorAllocated())
			return _stagingBufferList.emplace_back(renderer,dstBuffer,dstOffset,size);
		else
			if(_stagingBuffer._renderer==nullptr) {
				new(&_stagingBuffer) StagingBuffer(renderer,dstBuffer,dstOffset,size);
				return _stagingBuffer;
			}
			else {

				// create new list
				vector<StagingBuffer> l;
				l.reserve(2);
				l.emplace_back(move(_stagingBuffer));
				_stagingBuffer.~StagingBuffer();
				new(&_stagingBufferList) vector<StagingBuffer>();
				_stagingBufferList.swap(l);

				// create new buffer
				return _stagingBufferList.emplace_back(renderer,dstBuffer,dstOffset,size);
			}
}


StagingBuffer& StagingManager::createStagingBuffer(unsigned stagingBufferIndex,Renderer* renderer,vk::Buffer dstBuffer,size_t dstOffset,size_t size)
{
	if(!vectorAllocated())
		if(stagingBufferIndex==0) {
			if(_stagingBuffer._renderer==nullptr) {
				new(&_stagingBuffer) StagingBuffer(renderer,dstBuffer,dstOffset,size);
				return _stagingBuffer;
			}
			else {
				_stagingBuffer.reset(renderer,dstBuffer,dstOffset,size);
				return _stagingBuffer;
			}
		}
		else {
			setVectorAllocated(true);
			if(_stagingBuffer._renderer==nullptr) {

				// create list
				new(&_stagingBufferList) vector<StagingBuffer>();
				_stagingBufferList.reserve(stagingBufferIndex+1);

				// fill list
				for(size_t i=0; i<stagingBufferIndex; i++)
					_stagingBufferList.emplace_back(renderer);
				return _stagingBufferList.emplace_back(renderer,dstBuffer,dstOffset,size);
			}
			else {

				// create new list
				vector<StagingBuffer> l;
				l.reserve(stagingBufferIndex+1);
				l.emplace_back(move(_stagingBuffer));
				_stagingBuffer.~StagingBuffer();
				new(&_stagingBufferList) vector<StagingBuffer>();
				_stagingBufferList.swap(l);

				// fill list
				for(size_t i=1; i<stagingBufferIndex; i++)
					_stagingBufferList.emplace_back(renderer);
				return _stagingBufferList.emplace_back(renderer,dstBuffer,dstOffset,size);
			}
		}
	else {
		if(stagingBufferIndex<_stagingBufferList.size()) {
			_stagingBufferList[stagingBufferIndex].reset(renderer,dstBuffer,dstOffset,size);
			return _stagingBufferList[stagingBufferIndex];
		}
		else {
			while(_stagingBufferList.size()<stagingBufferIndex)
				_stagingBufferList.emplace_back(renderer);
			return _stagingBufferList.emplace_back(renderer,dstBuffer,dstOffset,size);
		}
	}
}


void StagingManager::record(vk::CommandBuffer commandBuffer)
{
	if(!vectorAllocated()) {
		if(_stagingBuffer._renderer!=nullptr)
			_stagingBuffer.record(commandBuffer);
	}
	else {
		for(StagingBuffer& sb : _stagingBufferList)
			sb.record(commandBuffer);
	}
}
