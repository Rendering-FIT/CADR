#include <CadR/StateSet.h>
#include <CadR/VulkanDevice.h>

using namespace CadR;


StateSet::~StateSet()
{
	clearCommandBuffers();
	if(_pipeline)
		(*_renderer->device())->destroy(_pipeline,nullptr,*_renderer->device());
}
