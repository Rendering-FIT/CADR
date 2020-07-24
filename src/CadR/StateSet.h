#pragma once

#include <vulkan/vulkan.hpp>
#include <boost/intrusive/list.hpp>
#include <CadR/ParentChildList.h>

namespace CadR {

class AttribStorage;
class Pipeline;
class Renderer;


class CADR_EXPORT StateSet final {
protected:
	Renderer* _renderer;
	uint32_t _id;
	size_t _numDrawCommands = 0;
	AttribStorage* _attribStorage = nullptr;
public:

	// pipeline to bind
	CadR::Pipeline* pipeline = nullptr;

	// descriptorSets to bind
	vk::PipelineLayout pipelineLayout;
	uint32_t descriptorSetNumber = 0;
	std::vector<vk::DescriptorSet> descriptorSets;
	std::vector<uint32_t> dynamicOffsets;

	// parent-child relation
	static const ParentChildListOffsets parentChildListOffsets;
	ChildList<StateSet,parentChildListOffsets> childList;
	ParentList<StateSet,parentChildListOffsets> parentList;

public:

	StateSet();
	StateSet(Renderer* renderer);
	StateSet(AttribStorage* attribStorage);
	StateSet(Renderer* renderer,AttribStorage* attribStorage);
	StateSet(const StateSet&) = delete;
	StateSet(StateSet&&) = delete;
	~StateSet();
	StateSet& operator=(const StateSet&) = delete;
	StateSet& operator=(StateSet&&) = delete;

	Renderer* renderer() const;
	uint32_t id() const;
	AttribStorage* attribStorage() const;

	size_t numDrawCommands() const;
	void setNumDrawCommands(size_t num);
	void incrementNumDrawCommands(ptrdiff_t increment);
	void decrementNumDrawCommands(ptrdiff_t decrement);

	void recordToCommandBuffer(vk::CommandBuffer cb,vk::DeviceSize& indirectBufferOffset) const;

	void setAttribStorage(AttribStorage* attribStorage);  ///< Sets the AttribStorage that will be bound when rendering this StateSet. It should not be changed if you have already Drawables using this StateSet as StateSet would then use different AttribStorage than Drawables leading to undefined behaviour.

};


}


// inline methods
#include <CadR/Renderer.h>
#include <cassert>
namespace CadR {

inline StateSet::StateSet() : _renderer(Renderer::get()), _attribStorage(nullptr)  { _id=_renderer->allocateStateSetId(); }
inline StateSet::StateSet(Renderer* renderer) : _renderer(renderer), _attribStorage(nullptr)  { _id=_renderer->allocateStateSetId(); }
inline StateSet::StateSet(AttribStorage* attribStorage)
	: _renderer(Renderer::get()), _attribStorage(attribStorage)  { _id=_renderer->allocateStateSetId(); }
inline StateSet::StateSet(Renderer* renderer,AttribStorage* attribStorage)
	: _renderer(renderer), _attribStorage(attribStorage)  { _id=_renderer->allocateStateSetId(); }
inline StateSet::~StateSet()  { assert(_numDrawCommands==0 && "Do not destroy StateSet while some DrawCommands still use it."); _renderer->releaseStateSetId(_id); }

inline Renderer* StateSet::renderer() const  { return _renderer; }
inline uint32_t StateSet::id() const  { return _id; }
inline AttribStorage* StateSet::attribStorage() const  { return _attribStorage; }

inline size_t StateSet::numDrawCommands() const  { return _numDrawCommands; }
inline void StateSet::setNumDrawCommands(size_t num)  { _numDrawCommands=num; }
inline void StateSet::incrementNumDrawCommands(ptrdiff_t increment)  { _numDrawCommands+=increment; }
inline void StateSet::decrementNumDrawCommands(ptrdiff_t decrement)  { _numDrawCommands-=decrement; }

inline void StateSet::setAttribStorage(AttribStorage* attribStorage)  { assert(_numDrawCommands==0 && "Can not change AttribStorage while there are already attached DrawCommands."); _attribStorage=attribStorage; }

}
