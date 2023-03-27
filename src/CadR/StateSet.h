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
	void appendDrawableUnsafe(Drawable& d, DrawableGpuData gpuData) noexcept;
	void removeDrawableUnsafe(Drawable& d) noexcept;
};


class CADR_EXPORT StateSet {
protected:

	Renderer* _renderer;  ///< Renderer associated with this Stateset.
	size_t _numDrawables = 0;  ///< The number of Drawables of this StateSet. It does not include Drawables of the child StateSets.
	bool _skipRecording;  ///< The flag optimizing the rendering, making this and all the child StateSets to be excluded from recording to the command buffer.
	                      ///< By default, it is set to true whenever there are no Drawables in this StateSet or any child StateSets. It can be forced to false by setting _forceRecording to true.
	bool _forceRecording;  ///< The flag forces recording to happen always, even if the StateSet does not contain any Drawables. However, some draw commands might be recorded by the user in the callbacks. 
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
	bool forceRecording() const;

	// drawable methods
	size_t numDrawables() const;
	void appendDrawable(Drawable& d, DrawableGpuData gpuData, uint32_t geometryMemoryId);
	void removeDrawable(Drawable& d);
	void appendDrawableUnsafe(Drawable& d, DrawableGpuData gpuData, uint32_t geometryMemoryId);
	void removeDrawableUnsafe(Drawable& d);

	// rendering methods
	size_t prepareRecording();
	void setForceRecording(bool value);  ///< Sets whether recording of this StateSet will always happen. It means that recordCallLists will be called, allowing the user to record its own draw commands. 
	                                     ///< If set to false, the recording will happen only if there are any Drawables in this StateSet or in any child StateSet. The recording can also be forced by requestRecording() on per-frame basis.
	void requestRecording();  ///< Requests the recording of this StateSet for the current frame even if it does not contain any Drawables. This function shall be called from prepareCallList callbacks only. Otherwise, it has no effect.
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
inline bool StateSet::forceRecording() const  { return _forceRecording; }

inline size_t StateSet::numDrawables() const  { return _numDrawables; }
inline void StateSet::appendDrawable(Drawable& d, DrawableGpuData gpuData, uint32_t geometryMemoryId)  { if(d._indexIntoStateSet!=~0u) removeDrawableUnsafe(d); appendDrawableUnsafe(d,gpuData,geometryMemoryId); }
inline void StateSet::removeDrawable(Drawable& d)  { if(d._indexIntoStateSet==~0u) return; removeDrawableUnsafe(d); d._indexIntoStateSet=~0u; }
inline void StateSet::removeDrawableUnsafe(Drawable& d)  { d._stateSetDrawableContainer->removeDrawableUnsafe(d); }

inline void StateSet::setForceRecording(bool value)  { _forceRecording = value; }
inline void StateSet::requestRecording()  { _skipRecording = false; }
inline void StateSet::setGeometryStorage(GeometryStorage* geometryStorage)  { assert(_numDrawables==0 && "Cannot change GeometryStorage while there are attached Drawables."); _geometryStorage=geometryStorage; }

}

#endif /* CADR_STATESET_H */
