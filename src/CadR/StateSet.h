// handle circular include
#include <CadR/Drawable.h>
#ifndef CADR_STATESET_H
#define CADR_STATESET_H

#include <vulkan/vulkan.hpp>
#include <CadR/ParentChildList.h>

namespace CadR {

class GeometryMemory;
class GeometryStorage;
class Pipeline;
class Renderer;


struct CADR_EXPORT StateSetDrawableContainer {

	StateSet* stateSet;
	GeometryMemory* geometryMemory;
	std::vector<DrawableGpuData> drawableDataList;  ///< List of Drawable data that is sent to GPU when Drawables are rendered.
	std::vector<Drawable*> drawablePtrList;  ///< List of Drawables attached to this StateSet.

	StateSetDrawableContainer(StateSet* s, GeometryMemory* m);
	void appendDrawableUnsafe(Drawable& d, DrawableGpuData gpuData);
	void removeDrawableUnsafe(Drawable& d);
};


class CADR_EXPORT StateSet {
protected:

	Renderer* _renderer;  ///< Renderer associated with this Stateset.
	bool _skipRecording;  ///< The flag optimizing the rendering. It is set by prepareRecording() and used by recordToCommandBuffer() to skip StateSets that does not contain any Drawables.
	size_t _numDrawables = 0;
	GeometryStorage* _geometryStorage = nullptr;  ///< GeometryStorage used by all Drawables attached to this StateSet.
	std::vector<std::unique_ptr<StateSetDrawableContainer>> _drawableContainerList;

public:

	// pipeline to bind
	CadR::Pipeline* pipeline = nullptr;

	// descriptorSets to bind
	vk::PipelineLayout pipelineLayout;
	uint32_t descriptorSetNumber = 0;
	std::vector<vk::DescriptorSet> descriptorSets;
	std::vector<uint32_t> dynamicOffsets;

	// list of functions that will be called during the preparation for StateSet's command buffer recording
	std::vector<std::function<void(StateSet*)>> prepareCallList;

	// list of functions that will be called during StateSet recording into the command buffer;
	// the function is called only if the StateSet is recorded, e.g. only if it contains any drawables
	std::vector<std::function<void(StateSet*, vk::CommandBuffer)>> recordCallList;

	// list of functions that will be called during recording of each
	// StateSetDrawableContainer of the StateSet into the command buffer;
	// the function is called only on containers that are recorded, e.g. only if they contain any drawables
	std::vector<std::function<void(StateSetDrawableContainer*, vk::CommandBuffer)>> recordContainerCallList;

	// parent-child relation
	static const ParentChildListOffsets parentChildListOffsets;
	ChildList<StateSet, parentChildListOffsets> childList;
	ParentList<StateSet, parentChildListOffsets> parentList;

public:

	// construction and destruction
	StateSet();
	StateSet(Renderer* renderer);
	StateSet(GeometryStorage* geometryStorage);
	StateSet(Renderer* renderer, GeometryStorage* geometryStorage);
	~StateSet();

	// deleted constructors and operators
	StateSet(const StateSet&) = delete;
	StateSet(StateSet&&) = delete;
	StateSet& operator=(const StateSet&) = delete;
	StateSet& operator=(StateSet&&) = delete;

	// getters
	Renderer* renderer() const;
	GeometryStorage* geometryStorage() const;
	const std::vector<std::unique_ptr<StateSetDrawableContainer>>& drawableContainerList() const;

	// drawable methods
	size_t numDrawables() const;
	void appendDrawable(Drawable& d, DrawableGpuData gpuData, uint32_t geometryMemoryId);
	void removeDrawable(Drawable& d);
	void appendDrawableUnsafe(Drawable& d, DrawableGpuData gpuData, uint32_t geometryMemoryId);
	void removeDrawableUnsafe(Drawable& d);

	// rendering methods
	size_t prepareRecording();
	void recordToCommandBuffer(vk::CommandBuffer cb, size_t& drawableCounter);

	// geometry storage
	void setGeometryStorage(GeometryStorage* geometryStorage);  ///< Sets the GeometryStorage that will be bound when rendering this StateSet. It must not be changed if you have already Drawables using this StateSet as StateSet would then use different GeometryStorage than Drawables, leading to undefined behaviour.

	friend Drawable;
	friend StateSetDrawableContainer;
};


}


// inline methods
#include <CadR/Renderer.h>
#include <cassert>
namespace CadR {

inline StateSetDrawableContainer::StateSetDrawableContainer(StateSet* s, GeometryMemory* m) : stateSet(s), geometryMemory(m)  {}

inline StateSet::StateSet() : _renderer(Renderer::get()), _geometryStorage(nullptr)  {}
inline StateSet::StateSet(Renderer* renderer) : _renderer(renderer), _geometryStorage(nullptr)  {}
inline StateSet::StateSet(GeometryStorage* geometryStorage)
	: _renderer(Renderer::get()), _geometryStorage(geometryStorage)  {}
inline StateSet::StateSet(Renderer* renderer, GeometryStorage* geometryStorage)
	: _renderer(renderer), _geometryStorage(geometryStorage)  {}
inline StateSet::~StateSet()  { assert(_numDrawables==0 && "Do not destroy StateSet while some Drawables still use it."); }

inline Renderer* StateSet::renderer() const  { return _renderer; }
inline GeometryStorage* StateSet::geometryStorage() const  { return _geometryStorage; }
inline const std::vector<std::unique_ptr<StateSetDrawableContainer>>& StateSet::drawableContainerList() const  { return _drawableContainerList; }

inline size_t StateSet::numDrawables() const  { return _numDrawables; }
inline void StateSet::appendDrawable(Drawable& d, DrawableGpuData gpuData, uint32_t geometryMemoryId)  { if(d._indexIntoStateSet!=~0u) removeDrawableUnsafe(d); appendDrawableUnsafe(d,gpuData,geometryMemoryId); }
inline void StateSet::removeDrawable(Drawable& d)  { if(d._indexIntoStateSet==~0u) return; removeDrawableUnsafe(d); d._indexIntoStateSet=~0u; }
inline void StateSet::removeDrawableUnsafe(Drawable& d)  { d._stateSetDrawableContainer->removeDrawableUnsafe(d); }

inline void StateSet::setGeometryStorage(GeometryStorage* geometryStorage)  { assert(_numDrawables==0 && "Cannot change GeometryStorage while there are attached Drawables."); _geometryStorage=geometryStorage; }

}

#endif /* CADR_STATESET_H */
