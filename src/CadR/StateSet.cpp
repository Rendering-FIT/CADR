#include <CadR/StateSet.h>
#include <CadR/Pipeline.h>
#include <CadR/VertexStorage.h>
#include <CadR/VulkanDevice.h>

using namespace std;
using namespace CadR;


const ParentChildListOffsets StateSet::parentChildListOffsets{
	size_t(&reinterpret_cast<StateSet*>(0)->parentList),
	size_t(&reinterpret_cast<StateSet*>(0)->childList)
};


void StateSet::removeDrawableUnsafe(Drawable& d)
{
	auto i=d._indexIntoStateSet;
	size_t lastIndex=_drawableDataList.size()-1;
	if(i==lastIndex) {
		_drawableDataList.pop_back();
		_drawableList.pop_back();
	}
	else {
		_drawableDataList[i]=_drawableDataList[lastIndex];
		_drawableDataList.pop_back();
		Drawable* movedDrawable=_drawableList[lastIndex];
		_drawableList[i]=movedDrawable;
		_drawableList.pop_back();
		movedDrawable->_indexIntoStateSet=i;
	}
}


size_t StateSet::prepareRecording()
{
	size_t numDrawables=_drawableDataList.size();
	for(StateSet& ss : childList)
		numDrawables+=ss.prepareRecording();
	_skipRecording=(numDrawables==0);
	return numDrawables;
}


void StateSet::recordToCommandBuffer(vk::CommandBuffer cmdBuffer,size_t& drawableCounter)
{
	// optimization
	// to not process StateSet subgraphs that does not have any Drawables
	if(_skipRecording)
		return;

	// bind pipeline
	VulkanDevice* device=_renderer->device();
	if(pipeline)
		device->cmdBindPipeline(cmdBuffer,vk::PipelineBindPoint::eGraphics,pipeline->get());

	// bind descriptor sets
	if(!descriptorSets.empty()) {
		device->cmdBindDescriptorSets(
			cmdBuffer,  // commandBuffer
			vk::PipelineBindPoint::eGraphics,  // pipelineBindPoint
			pipelineLayout,  // layout
			descriptorSetNumber,  // firstSet
			descriptorSets,  // descriptorSets
			dynamicOffsets  // dynamicOffsets
		);
	}

	// call user-registered functions
	for(auto& f : callList)
		f(this,cmdBuffer);

	if(numDrawables()!=0) {

		assert(_vertexStorage && "VertexStorage have to be assigned before calling StateSet::recordToCommandBuffer() if StateSet has associated Drawables.");

		// copy drawable data
		memcpy(
			&_renderer->drawableStagingData()[drawableCounter],  // dst
			_drawableDataList.data(),  // src
			_drawableDataList.size()*sizeof(DrawableGpuData)  // size
		);

		// bind attributes
		size_t numAttributes=_vertexStorage->bufferList().size();
		vector<vk::DeviceSize> zeros(numAttributes,0);
		device->cmdBindVertexBuffers(
			cmdBuffer,  // commandBuffer
			0,  // firstBinding
			uint32_t(numAttributes),  // bindingCount
			_vertexStorage->bufferList().data(),  // pBuffers
			zeros.data()  // pOffsets
		);

		// draw command
		device->cmdDrawIndexedIndirect(
			cmdBuffer,  // commandBuffer
			_renderer->drawIndirectBuffer(),  // buffer
			drawableCounter*sizeof(vk::DrawIndexedIndirectCommand),  // offset
			uint32_t(numDrawables()),  // drawCount
			sizeof(vk::DrawIndexedIndirectCommand)  // stride
		);

		// update drawableCounter
		drawableCounter+=numDrawables();
	}

	// record child StateSets
	for(StateSet& child : childList)
		child.recordToCommandBuffer(cmdBuffer,drawableCounter);
}
