#pragma once

#include <CadR/Export.h>
#include <vulkan/vulkan.hpp>

namespace CadR {

class AttribStorage;
class Renderer;


class CADR_EXPORT StateSet final {
protected:
	Renderer* _renderer;
	uint32_t _id;
	size_t _numDrawCommands = 0;
	AttribStorage* _attribStorage = nullptr;
	vk::Pipeline _pipeline;
public:

	StateSet();
	StateSet(Renderer* renderer);
	StateSet(AttribStorage* attribStorage,vk::Pipeline pipeline);
	StateSet(Renderer* renderer,AttribStorage* attribStorage,vk::Pipeline pipeline);
	StateSet(const StateSet&) = delete;
	StateSet(StateSet&&) noexcept;
	~StateSet();
	StateSet& operator=(const StateSet&) = delete;
	StateSet& operator=(StateSet&& rhs) noexcept;

	Renderer* renderer() const;
	uint32_t id() const;
	AttribStorage* attribStorage() const;
	vk::Pipeline pipeline() const;

	size_t numDrawCommands() const;
	void setNumDrawCommands(size_t num);
	void incrementNumDrawCommands(ptrdiff_t increment);
	void decrementNumDrawCommands(ptrdiff_t decrement);

	void recordToCommandBuffer(vk::CommandBuffer cb,vk::DeviceSize& indirectBufferOffset) const;

	void setAttribStorage(AttribStorage* attribStorage);  ///< Sets the AttribStorage that will be bound when rendering this StateSet. It should not be changed if you have already Drawables using this StateSet as StateSet would then use different AttribStorage than Drawables leading to undefined behaviour.
	void setPipeline(vk::Pipeline pipeline);

protected:
	void cleanUp() noexcept;
};


}


// inline methods
#include <CadR/Renderer.h>
#include <CadR/VulkanDevice.h>
#include <cassert>
namespace CadR {

inline StateSet::StateSet() : _renderer(Renderer::get()), _attribStorage(nullptr)  { _id=_renderer->allocateStateSetId(); }
inline StateSet::StateSet(Renderer* renderer) : _renderer(renderer), _attribStorage(nullptr)  { _id=_renderer->allocateStateSetId(); }
inline StateSet::StateSet(AttribStorage* attribStorage,vk::Pipeline pipeline)
	: _renderer(Renderer::get()), _attribStorage(attribStorage), _pipeline(pipeline)  { _id=_renderer->allocateStateSetId(); }
inline StateSet::StateSet(Renderer* renderer,AttribStorage* attribStorage,vk::Pipeline pipeline)
	: _renderer(renderer), _attribStorage(attribStorage), _pipeline(pipeline)  { _id=_renderer->allocateStateSetId(); }
inline StateSet::StateSet(StateSet&& other) noexcept : _renderer(other._renderer), _attribStorage(other._attribStorage), _pipeline(other._pipeline)  { other._pipeline=nullptr; _id=other._id; other._id=_renderer->allocateStateSetId(); }
inline StateSet& StateSet::operator=(StateSet&& rhs) noexcept  { cleanUp(); _renderer=rhs._renderer; _attribStorage=rhs._attribStorage; _pipeline=rhs._pipeline; rhs._pipeline=nullptr; return *this; }
inline StateSet::~StateSet()  { cleanUp(); _renderer->releaseStateSetId(_id); assert(_numDrawCommands==0 && "StateSet must not be destroyed while some DrawCommands still use it."); }

inline Renderer* StateSet::renderer() const  { return _renderer; }
inline uint32_t StateSet::id() const  { return _id; }
inline AttribStorage* StateSet::attribStorage() const  { return _attribStorage; }
inline vk::Pipeline StateSet::pipeline() const  { return _pipeline; }

inline size_t StateSet::numDrawCommands() const  { return _numDrawCommands; }
inline void StateSet::setNumDrawCommands(size_t num)  { _numDrawCommands=num; }
inline void StateSet::incrementNumDrawCommands(ptrdiff_t increment)  { _numDrawCommands+=increment; }
inline void StateSet::decrementNumDrawCommands(ptrdiff_t decrement)  { _numDrawCommands-=decrement; }

inline void StateSet::setAttribStorage(AttribStorage* attribStorage)  { assert(_numDrawCommands==0 && "Can not change AttribStorage while there are already attached DrawCommands."); _attribStorage=attribStorage; }
inline void StateSet::setPipeline(vk::Pipeline pipeline)  { if(_pipeline) (*_renderer->device())->destroy(_pipeline,nullptr,*_renderer->device()); _pipeline=pipeline; }

}
