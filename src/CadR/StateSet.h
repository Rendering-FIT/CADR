// handle circular include
#include <CadR/Drawable.h>
#ifndef CADR_STATESET_H
#define CADR_STATESET_H

#include <vulkan/vulkan.hpp>
#include <boost/intrusive/list.hpp>
#include <CadR/ParentChildList.h>

namespace CadR {

class Pipeline;
class Renderer;
class VertexStorage;

class CADR_EXPORT StateSet final {
protected:
	Renderer* _renderer;  ///< Renderer associated with this Stateset.
	bool _skipRecording;  ///< The flag optimizing the rendering. It is set by prepareRecording() and used by recordToCommandBuffer() to skip StateSets that does not contain any Drawables.
	std::vector<DrawableGpuData> _drawableDataList;  ///< List of Drawable data that is sent to GPU when Drawables are rendered.
	std::vector<Drawable*> _drawableList;  ///< List of Drawables attached to this StateSet.
	VertexStorage* _vertexStorage = nullptr;  ///< VertexStorage used by all Drawables attached to this StateSet.
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
	VertexStorage* vertexStorage() const;

	size_t numDrawables() const;
	void appendDrawable(Drawable& d,DrawableGpuData gpuData);
	void removeDrawable(Drawable& d);
	void appendDrawableUnsafe(Drawable& d,DrawableGpuData gpuData);
	void removeDrawableUnsafe(Drawable& d);

	size_t prepareRecording();
	void recordToCommandBuffer(vk::CommandBuffer cb,size_t& drawableCounter);

	void setVertexStorage(VertexStorage* vertexStorage);  ///< Sets the VertexStorage that will be bound when rendering this StateSet. It should not be changed if you have already Drawables using this StateSet as StateSet would then use different VertexStorage than Drawables leading to undefined behaviour.

	friend Drawable;
};


}


// inline methods
#include <CadR/Renderer.h>
#include <cassert>
namespace CadR {

inline StateSet::StateSet() : _renderer(Renderer::get()), _vertexStorage(nullptr)  {}
inline StateSet::StateSet(Renderer* renderer) : _renderer(renderer), _vertexStorage(nullptr)  {}
inline StateSet::StateSet(VertexStorage* vertexStorage)
	: _renderer(Renderer::get()), _vertexStorage(vertexStorage)  {}
inline StateSet::StateSet(Renderer* renderer,VertexStorage* vertexStorage)
	: _renderer(renderer), _vertexStorage(vertexStorage)  {}
inline StateSet::~StateSet()  { assert(_drawableDataList.empty() && "Do not destroy StateSet while some Drawables still use it."); }

inline Renderer* StateSet::renderer() const  { return _renderer; }
inline VertexStorage* StateSet::vertexStorage() const  { return _vertexStorage; }

inline size_t StateSet::numDrawables() const  { return _drawableDataList.size(); }
inline void StateSet::appendDrawableUnsafe(Drawable& d,DrawableGpuData gpuData)  { d._stateSet=this; d._indexIntoStateSet=uint32_t(_drawableDataList.size()); _drawableDataList.emplace_back(gpuData); _drawableList.emplace_back(&d); }
inline void StateSet::appendDrawable(Drawable& d,DrawableGpuData gpuData)  { if(d._indexIntoStateSet!=~0) removeDrawableUnsafe(d); appendDrawableUnsafe(d,gpuData); }
inline void StateSet::removeDrawable(Drawable& d)  { if(d._indexIntoStateSet==~0) return; removeDrawableUnsafe(d); d._indexIntoStateSet=~0; }

inline void StateSet::setVertexStorage(VertexStorage* vertexStorage)  { assert(_drawableDataList.empty() && "Cannot change VertexStorage while there are attached Drawables."); _vertexStorage=vertexStorage; }

}

#endif /* CADR_STATESET_H */
