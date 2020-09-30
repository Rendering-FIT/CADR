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


void StateSet::recordToCommandBuffer(vk::CommandBuffer cb,vk::DeviceSize& indirectBufferOffset) const
{
	// optimization (need not to be here if you do not like it)
	if(_numDrawables==0 && childList.empty())
		return;

	// bind pipeline
	VulkanDevice* device=_renderer->device();
	if(pipeline)
		device->cmdBindPipeline(cb,vk::PipelineBindPoint::eGraphics,pipeline->get());

	// bind descriptor sets
	if(!descriptorSets.empty()) {
		device->cmdBindDescriptorSets(
			cb,  // commandBuffer
			vk::PipelineBindPoint::eGraphics,  // pipelineBindPoint
			pipelineLayout,  // layout
			descriptorSetNumber,  // firstSet
			descriptorSets,  // descriptorSets
			dynamicOffsets  // dynamicOffsets
		);
	}

	if(_numDrawables!=0) {

		assert(_vertexStorage && "VertexStorage have to be assigned before calling StateSet::recordToCommandBuffer() if StateSet has associated Drawables.");

		// bind attributes
		size_t numAttributes=_vertexStorage->bufferList().size();
		vector<vk::DeviceSize> zeros(numAttributes,0);
		device->cmdBindVertexBuffers(
			cb,  // commandBuffer
			0,  // firstBinding
			uint32_t(numAttributes),  // bindingCount
			_vertexStorage->bufferList().data(),  // pBuffers
			zeros.data()  // pOffsets
		);

		// draw command
		device->cmdDrawIndexedIndirect(
			cb,  // commandBuffer
			_renderer->drawIndirectBuffer(),  // buffer
			indirectBufferOffset,  // offset
			uint32_t(_numDrawables),  // drawCount
			sizeof(vk::DrawIndexedIndirectCommand)  // stride
		);

		// update rendering data
		_renderer->stateSetStagingData()[_id]=uint32_t(indirectBufferOffset/4);
		indirectBufferOffset+=_numDrawables*sizeof(vk::DrawIndexedIndirectCommand);
	}

	// record child StateSets
	for(const StateSet& child : childList)
		child.recordToCommandBuffer(cb, indirectBufferOffset);
}
