#include <CadR/RenderPass.h>
#include <CadR/StateSet.h>
#include <CadR/VulkanDevice.h>

using namespace CadR;


#if 0
void RenderPass::destroy()
{
	_renderer->device()->destroy(_renderPass);
}


void RenderPass::recordToCommandBuffer(vk::CommandBuffer cb,vk::DeviceSize& indirectBufferOffset) const
{
	VulkanDevice* device=_renderer->device();
	for(auto* ss:stateSetList) {

		// bind pipeline
		device->cmdBindPipeline(cb,vk::PipelineBindPoint::eGraphics,ss->pipeline()->get());

		// record command buffer
		ss->recordToCommandBuffer(cb,indirectBufferOffset);
	}
}
#endif
