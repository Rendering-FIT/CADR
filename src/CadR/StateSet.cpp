#include <CadR/StateSet.h>
#include <CadR/AttribStorage.h>
#include <CadR/Pipeline.h>
#include <CadR/VulkanDevice.h>

using namespace std;
using namespace CadR;


const ParentChildListOffsets StateSet::parentChildListOffsets{
	size_t(&reinterpret_cast<StateSet*>(0)->parentList),
	size_t(&reinterpret_cast<StateSet*>(0)->childList)
};


void StateSet::recordToCommandBuffer(vk::CommandBuffer cb,vk::DeviceSize& indirectBufferOffset) const
{
	assert(_pipeline && "Pipeline have to be assigned before calling StateSet::recordToCommandBuffer().");

	// optimization (need not to be here if you do not like it)
	if(_numDrawCommands==0 && childList.empty())
		return;

	// bind descriptor sets
	VulkanDevice* device=_renderer->device();
	if(!descriptorSets.empty()) {
		device->cmdBindDescriptorSets(
			cb,  // commandBuffer
			vk::PipelineBindPoint::eGraphics,  // pipelineBindPoint
			_pipeline->layout(),  // layout
			firstDescriptorSet,  // firstSet
			descriptorSets,  // descriptorSets
			dynamicOffsets  // dynamicOffsets
		);
	}

	if(_numDrawCommands!=0) {

		assert(_attribStorage && "AttribStorage have to be assigned before calling StateSet::recordToCommandBuffer() if StateSet has associated DrawCommands.");

		// bind attributes
		size_t numAttributes=_attribStorage->bufferList().size();
		vector<vk::DeviceSize> zeros(numAttributes,0);
		device->cmdBindVertexBuffers(
			cb,  // commandBuffer
			0,  // firstBinding
			uint32_t(numAttributes),  // bindingCount
			_attribStorage->bufferList().data(),  // pBuffers
			zeros.data()  // pOffsets
		);

		// draw command
		device->cmdDrawIndexedIndirect(
			cb,  // commandBuffer
			_renderer->drawIndirectBuffer(),  // buffer
			indirectBufferOffset,  // offset
			uint32_t(_numDrawCommands),  // drawCount
			sizeof(vk::DrawIndexedIndirectCommand)  // stride
		);

		// update rendering data
		_renderer->stateSetStagingData()[_id]=uint32_t(indirectBufferOffset/4);
		indirectBufferOffset+=_numDrawCommands*sizeof(vk::DrawIndexedIndirectCommand);
	}

	// record child StateSets
	for(const StateSet& child : childList)
		child.recordToCommandBuffer(cb, indirectBufferOffset);
}
