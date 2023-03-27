#include <CadR/StateSet.h>
#include <CadR/GeometryMemory.h>
#include <CadR/GeometryStorage.h>
#include <CadR/Pipeline.h>
#include <CadR/VulkanDevice.h>

using namespace std;
using namespace CadR;


const ParentChildListOffsets StateSet::parentChildListOffsets{
	size_t(&reinterpret_cast<StateSet*>(0)->parentList),
	size_t(&reinterpret_cast<StateSet*>(0)->childList)
};


void StateSetDrawableContainer::appendDrawableUnsafe(Drawable& d, DrawableGpuData gpuData) noexcept
{
	d._stateSetDrawableContainer = this;
	d._indexIntoStateSet = uint32_t(drawableDataList.size());
	drawableDataList.emplace_back(gpuData);
	drawablePtrList.emplace_back(&d);
	stateSet->_numDrawables++;
}


void StateSetDrawableContainer::removeDrawableUnsafe(Drawable& d) noexcept
{
	uint32_t i = d._indexIntoStateSet;
	size_t lastIndex = drawableDataList.size()-1;
	if(i == lastIndex) {
		// remove last element
		drawableDataList.pop_back();
		drawablePtrList.pop_back();
	}
	else {
		// remove element by replacing it with the last element
		drawableDataList[i] = drawableDataList[lastIndex];
		drawableDataList.pop_back();
		Drawable* movedDrawable = drawablePtrList[lastIndex];
		drawablePtrList[i] = movedDrawable;
		drawablePtrList.pop_back();
		movedDrawable->_indexIntoStateSet = i;
	}
	stateSet->_numDrawables--;
}


void StateSet::appendDrawableUnsafe(Drawable& d, DrawableGpuData gpuData, uint32_t geometryMemoryId)
{
	// make sure we have enough StateSet::DrawableContainers
	if(_drawableContainerList.size() <= geometryMemoryId) {
		_drawableContainerList.reserve(geometryMemoryId+1);
		while(_drawableContainerList.size() <= geometryMemoryId)
			_drawableContainerList.emplace_back(make_unique<StateSetDrawableContainer>(
				this, _geometryStorage->geometryMemoryList()[_drawableContainerList.size()].get()));
	}

	// append Drawable into StateSetDrawableContainer
	_drawableContainerList[geometryMemoryId]->appendDrawableUnsafe(d, gpuData);
}


size_t StateSet::prepareRecording()
{
	// call user-registered functions
	_skipRecording = !_forceRecording;
	for(auto& f : prepareCallList)
		f(this);

	// recursively call child-StateSets and process number of drawables
	size_t numDrawables = _numDrawables;
	for(StateSet& ss : childList) {
		numDrawables += ss.prepareRecording();
		_skipRecording = _skipRecording && ss._skipRecording;
	}
	_skipRecording = _skipRecording && (numDrawables == 0);
	return numDrawables;
}


void StateSet::recordToCommandBuffer(vk::CommandBuffer commandBuffer, size_t& drawableCounter)
{
	// optimization
	// to not process StateSet subgraphs that does not have any Drawables
	if(_skipRecording)
		return;

	// bind pipeline
	VulkanDevice* device = _renderer->device();
	if(pipeline)
		device->cmdBindPipeline(commandBuffer, vk::PipelineBindPoint::eGraphics, pipeline->get());

	// bind descriptor sets
	if(!descriptorSets.empty()) {
		device->cmdBindDescriptorSets(
			commandBuffer,  // commandBuffer
			vk::PipelineBindPoint::eGraphics,  // pipelineBindPoint
			pipelineLayout,  // layout
			descriptorSetNumber,  // firstSet
			descriptorSets,  // descriptorSets
			dynamicOffsets  // dynamicOffsets
		);
	}

	// call user-registered functions
	for(auto& f : recordCallList)
		f(this, commandBuffer);

	if(_numDrawables != 0) {

		assert(_geometryStorage && "If StateSet has associated Drawables, GeometryStorage have to be assigned before calling StateSet::recordToCommandBuffer().");

		for(unique_ptr<StateSetDrawableContainer>& uniqueContainer : _drawableContainerList) {
			StateSetDrawableContainer* container = uniqueContainer.get();

			// get numDrawables
			size_t numDrawables = container->drawableDataList.size();
			if(numDrawables == 0)
				continue;

			// call user-registered functions
			for(auto& f : recordContainerCallList)
				f(container, commandBuffer);

			// copy drawable data
			memcpy(
				&_renderer->drawableStagingData()[drawableCounter],  // dst
				container->drawableDataList.data(),  // src
				numDrawables*sizeof(DrawableGpuData)  // size
			);

			// bind attributes
			GeometryMemory* m = container->geometryMemory;
			size_t numAttribs = m->numAttribs();
			if(numAttribs > 0) {
				vector<vk::Buffer> buffers(numAttribs, m->buffer());
				device->cmdBindVertexBuffers(
					commandBuffer,  // commandBuffer
					0,  // firstBinding
					uint32_t(numAttribs),  // bindingCount
					buffers.data(),  // pBuffers
					m->attribOffsets().data()  // pOffsets
				);
			}

			// bind indices
			device->cmdBindIndexBuffer(
				commandBuffer,  // commandBuffer
				m->buffer(),  // buffer
				m->indexOffset(),  // offset
				vk::IndexType::eUint32  // indexType
			);

			// draw command
			device->cmdDrawIndexedIndirect(
				commandBuffer,  // commandBuffer
				_renderer->drawIndirectBuffer(),  // buffer
				_renderer->drawIndirectCommandOffset() +  // offset
					drawableCounter * sizeof(vk::DrawIndexedIndirectCommand),
				uint32_t(numDrawables),  // drawCount
				sizeof(vk::DrawIndexedIndirectCommand)  // stride
			);

			// update drawableCounter
			drawableCounter += numDrawables;
		}
	}

	// record child StateSets
	for(StateSet& child : childList)
		child.recordToCommandBuffer(commandBuffer, drawableCounter);
}
