#include <CadR/StateSet.h>
#include <CadR/AttribStorage.h>
#include <CadR/VulkanDevice.h>
#include <vector>

using namespace std;
using namespace CadR;


void StateSet::cleanUp() noexcept
{
	VulkanDevice* device=_renderer->device();
	(*device)->destroy(_pipeline,nullptr,*device);
	_pipeline=nullptr;
}


void StateSet::recordToCommandBuffer(vk::CommandBuffer cb,vk::DeviceSize& indirectBufferOffset) const
{
	assert(_attribStorage && "AttribStorage have to be assigned before calling StateSet::recordToCommandBuffer().");
	assert(_pipeline && "Pipeline have to be assigned before calling StateSet::recordToCommandBuffer().");

	// optimization (need not to be here if you do not like it)
	if(_numDrawCommands==0)
		return;

	// bind pipeline
	VulkanDevice* device=_renderer->device();
	cb.bindPipeline(vk::PipelineBindPoint::eGraphics,_pipeline,*device);

	// bind attributes
	size_t numAttributes=_attribStorage->bufferList().size();
	vector<vk::DeviceSize> zeros(numAttributes,0);
	cb.bindVertexBuffers(
		0,  // firstBinding
		uint32_t(numAttributes),  // bindingCount
		_attribStorage->bufferList().data(),  // pBuffers
		zeros.data(),  // pOffsets
		*device  // dispatch
	);

	// draw command
	cb.drawIndexedIndirect(
		_renderer->drawIndirectBuffer(),  // buffer
		indirectBufferOffset,  // offset
		uint32_t(_numDrawCommands),  // drawCount
		sizeof(vk::DrawIndexedIndirectCommand),  // stride
		*device  // dispatch
	);

	// update rendering data
	_renderer->stateSetStagingData()[_id]=indirectBufferOffset/4;
	indirectBufferOffset+=_numDrawCommands*sizeof(vk::DrawIndexedIndirectCommand);
}
