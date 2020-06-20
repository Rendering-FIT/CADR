#include <CadR/Pipeline.h>
#include <CadR/Renderer.h>
#include <CadR/VulkanDevice.h>

using namespace CadR;


Pipeline::~Pipeline()
{
}


void Pipeline::destroy()
{
	VulkanDevice* device=_renderer->device();
	for(auto& d : _descriptorSetLayouts)
		device->destroy(d);
	_descriptorSetLayouts.clear();
	device->destroy(_pipelineLayout);
	device->destroy(_pipeline);
}
