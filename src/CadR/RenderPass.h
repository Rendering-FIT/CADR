#pragma once

#include <vulkan/vulkan.hpp>
#include <boost/intrusive/list.hpp>
#include <list>

namespace CadR {

class Renderer;
class StateSet;


class CADR_EXPORT RenderPass final {
protected:
	Renderer* _renderer;
	vk::RenderPass _renderPass;
public:
	boost::intrusive::list_member_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink>
	> _renderPassListHook;  // Hook to Renderer::renderPassList.
public:
	std::list<StateSet*> stateSetList;
public:

	RenderPass();
	RenderPass(Renderer* renderer);
	RenderPass(vk::RenderPass renderPass);
	RenderPass(Renderer* renderer,vk::RenderPass renderPass);
	RenderPass(const RenderPass&) = delete;
	RenderPass(RenderPass&&) = default;
	~RenderPass();
	RenderPass& operator=(const RenderPass&) = delete;
	RenderPass& operator=(RenderPass&&) = default;

	void destroy();

	Renderer* renderer() const;
	vk::RenderPass get() const;
	void set(vk::RenderPass renderPass);

	void recordToCommandBuffer(vk::CommandBuffer cb,vk::DeviceSize& indirectBufferOffset) const;

};


}


// inline methods
#include <CadR/Renderer.h>
namespace CadR {

inline RenderPass::RenderPass() : _renderer(Renderer::get())  { _renderer->renderPassList().push_back(*this); }
inline RenderPass::RenderPass(Renderer* renderer) : _renderer(renderer)  { _renderer->renderPassList().push_back(*this); }
inline RenderPass::RenderPass(vk::RenderPass renderPass) : _renderer(Renderer::get()), _renderPass(renderPass)  { _renderer->renderPassList().push_back(*this); }
inline RenderPass::RenderPass(Renderer* renderer,vk::RenderPass renderPass) : _renderer(renderer), _renderPass(renderPass)  { _renderer->renderPassList().push_back(*this); }
inline RenderPass::~RenderPass()  {}

inline Renderer* RenderPass::renderer() const  { return _renderer; }
inline vk::RenderPass RenderPass::get() const  { return _renderPass; }
inline void RenderPass::set(vk::RenderPass renderPass)  { _renderPass=renderPass; }

}
