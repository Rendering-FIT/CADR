#pragma once

#include <vulkan/vulkan.hpp>
#include <boost/intrusive/list.hpp>
#include <CadR/ParentChildList.h>

namespace CadR {

class Pipeline;
class Renderer;
class VertexStorage;


class CADR_EXPORT StateSet final {
protected:
	Renderer* _renderer;
	uint32_t _id;
	size_t _numDrawables = 0;
	VertexStorage* _vertexStorage = nullptr;
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
	StateSet(VertexStorage* vertexStorage);
	StateSet(Renderer* renderer,VertexStorage* vertexStorage);
	StateSet(const StateSet&) = delete;
	StateSet(StateSet&&) = delete;
	~StateSet();
	StateSet& operator=(const StateSet&) = delete;
	StateSet& operator=(StateSet&&) = delete;

	Renderer* renderer() const;
	uint32_t id() const;
	VertexStorage* vertexStorage() const;

	size_t numDrawables() const;
	void setNumDrawables(size_t num);
	void incrementNumDrawables(ptrdiff_t increment=1);
	void decrementNumDrawables(ptrdiff_t decrement=1);

	void recordToCommandBuffer(vk::CommandBuffer cb,vk::DeviceSize& indirectBufferOffset) const;

	void setVertexStorage(VertexStorage* vertexStorage);  ///< Sets the VertexStorage that will be bound when rendering this StateSet. It should not be changed if you have already Drawables using this StateSet as StateSet would then use different VertexStorage than Drawables leading to undefined behaviour.

};


}


// inline methods
#include <CadR/Renderer.h>
#include <cassert>
namespace CadR {

inline StateSet::StateSet() : _renderer(Renderer::get()), _vertexStorage(nullptr)  { _id=_renderer->allocateStateSetId(); }
inline StateSet::StateSet(Renderer* renderer) : _renderer(renderer), _vertexStorage(nullptr)  { _id=_renderer->allocateStateSetId(); }
inline StateSet::StateSet(VertexStorage* vertexStorage)
	: _renderer(Renderer::get()), _vertexStorage(vertexStorage)  { _id=_renderer->allocateStateSetId(); }
inline StateSet::StateSet(Renderer* renderer,VertexStorage* vertexStorage)
	: _renderer(renderer), _vertexStorage(vertexStorage)  { _id=_renderer->allocateStateSetId(); }
inline StateSet::~StateSet()  { assert(_numDrawables==0 && "Do not destroy StateSet while some Drawables still use it."); _renderer->releaseStateSetId(_id); }

inline Renderer* StateSet::renderer() const  { return _renderer; }
inline uint32_t StateSet::id() const  { return _id; }
inline VertexStorage* StateSet::vertexStorage() const  { return _vertexStorage; }

inline size_t StateSet::numDrawables() const  { return _numDrawables; }
inline void StateSet::setNumDrawables(size_t num)  { _numDrawables=num; }
inline void StateSet::incrementNumDrawables(ptrdiff_t increment)  { _numDrawables+=increment; }
inline void StateSet::decrementNumDrawables(ptrdiff_t decrement)  { _numDrawables-=decrement; }

inline void StateSet::setVertexStorage(VertexStorage* vertexStorage)  { assert(_numDrawables==0 && "Cannot change VertexStorage while there are attached Drawables."); _vertexStorage=vertexStorage; }

}
