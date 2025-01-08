#include <CadR/StateSet.h>
#include <CadR/Pipeline.h>
#include <CadR/Renderer.h>
#include <CadR/VulkanDevice.h>

using namespace std;
using namespace CadR;


const ParentChildListOffsets StateSet::parentChildListOffsets{
	size_t(&reinterpret_cast<StateSet*>(0)->parentList),
	size_t(&reinterpret_cast<StateSet*>(0)->childList)
};


void StateSet::appendDrawableInternal(Drawable& d, DrawableGpuData gpuData)
{
	d._stateSet = this;
	d._indexIntoStateSet = uint32_t(_drawableDataList.size());
	_drawableDataList.emplace_back(gpuData);
	_drawablePtrList.emplace_back(&d);
}


void StateSet::removeDrawableInternal(Drawable& d) noexcept
{
	uint32_t i = d._indexIntoStateSet;
	size_t lastIndex = _drawableDataList.size()-1;
	if(i == lastIndex) {
		// remove last element
		_drawableDataList.pop_back();
		_drawablePtrList.pop_back();
	}
	else {
		// remove element by replacing it with the last element
		_drawableDataList[i] = _drawableDataList[lastIndex];
		_drawableDataList.pop_back();
		Drawable* movedDrawable = _drawablePtrList[lastIndex];
		_drawablePtrList[i] = movedDrawable;
		_drawablePtrList.pop_back();
		movedDrawable->_indexIntoStateSet = i;
	}
}


void StateSet::removeAllDrawables() noexcept
{
	for(Drawable* d : _drawablePtrList)
		d->_indexIntoStateSet = ~0u;
	_drawableDataList.clear();
	_drawablePtrList.clear();
}


size_t StateSet::prepareRecording()
{
	// call user-registered functions
	_skipRecording = !_forceRecording;
	for(auto& f : prepareCallList)
		f(*this);

	// recursively call child-StateSets and process number of drawables
	size_t numDrawables = _drawableDataList.size();
	for(StateSet& ss : childList) {
		numDrawables += ss.prepareRecording();
		_skipRecording = _skipRecording && ss._skipRecording;
	}
	_skipRecording = _skipRecording && (numDrawables == 0);
	return numDrawables;
}


void StateSet::recordToCommandBuffer(vk::CommandBuffer commandBuffer, vk::PipelineLayout currentPipelineLayout, size_t& drawableCounter)
{
	// optimization
	// to not process StateSet subgraphs that does not have any Drawables
	if(_skipRecording)
		return;

	// bind pipeline
	VulkanDevice& device = _renderer->device();
	if(pipeline) {
		device.cmdBindPipeline(commandBuffer, vk::PipelineBindPoint::eGraphics, pipeline->get());
		currentPipelineLayout = pipeline->layout();
	}

	// set current pipeline layout
	if(pipelineLayout)
		currentPipelineLayout = pipelineLayout;

	// bind descriptor sets
	if(!descriptorSets.empty()) {
		device.cmdBindDescriptorSets(
			commandBuffer,  // commandBuffer
			vk::PipelineBindPoint::eGraphics,  // pipelineBindPoint
			currentPipelineLayout,  // layout
			descriptorSetNumber,  // firstSet
			descriptorSets,  // descriptorSets
			dynamicOffsets  // dynamicOffsets
		);
	}

	// call user-registered functions
	for(auto& f : recordCallList)
		f(*this, commandBuffer);

	size_t numDrawables = _drawableDataList.size();
	if(numDrawables > 0) {

		// copy drawable data
		memcpy(
			&_renderer->drawableStagingData()[drawableCounter],  // dst
			_drawableDataList.data(),  // src
			numDrawables*sizeof(DrawableGpuData)  // size
		);

		// update payload address
		device.cmdPushConstants(
			commandBuffer,  // commandBuffer
			currentPipelineLayout,  // pipelineLayout
			vk::ShaderStageFlagBits::eVertex,  // stageFlags
			0,  // offset
			sizeof(uint64_t),  // size
			array<uint64_t,1>{  // pValues
				_renderer->drawablePayloadDeviceAddress() + (drawableCounter * Renderer::drawablePayloadRecordSize),  // payloadBufferPtr
			}.data()
		);

		// draw command
		device.cmdDrawIndirect(
			commandBuffer,  // commandBuffer
			_renderer->drawIndirectBuffer(),  // buffer
			drawableCounter * sizeof(vk::DrawIndirectCommand),  // offset
			uint32_t(numDrawables),  // drawCount
			sizeof(vk::DrawIndirectCommand)  // stride
		);

		// update drawableCounter
		drawableCounter += numDrawables;

	}

	// record child StateSets
	for(StateSet& child : childList)
		child.recordToCommandBuffer(commandBuffer, currentPipelineLayout, drawableCounter);
}
