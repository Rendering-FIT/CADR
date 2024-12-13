#ifndef CADR_STATE_SET_HEADER
# define CADR_STATE_SET_HEADER

# ifndef CADR_NO_INLINE_FUNCTIONS
#  define CADR_NO_INLINE_FUNCTIONS
#  include <CadR/Drawable.h>
#  include <CadR/ParentChildList.h>
#  undef CADR_NO_INLINE_FUNCTIONS
# else
#  include <CadR/Drawable.h>
#  include <CadR/ParentChildList.h>
# endif
# include <functional>

namespace CadR {

class Pipeline;
class Renderer;


class CADR_EXPORT StateSet {
protected:

	Renderer* _renderer;  ///< Renderer associated with this Stateset.
	bool _skipRecording;  ///< The flag optimizing the rendering. When set, it excludes this and all the child StateSets from recording into the command buffer.
	                      ///< By default, it is set to true whenever there are no Drawables in this StateSet or any child StateSets. It can be forced to false by setting _forceRecording to true.
	bool _forceRecording = false;  ///< The flag forces recording to happen always, even if the StateSet does not contain any Drawables. This might be useful as some draw commands might be recorded by the user in the callbacks. 
	std::vector<DrawableGpuData> _drawableDataList;  ///< List of Drawable data that is sent to GPU when Drawables are rendered.
	std::vector<Drawable*> _drawablePtrList;  ///< List of Drawables attached to this StateSet.

public:

	// pipeline to bind
	CadR::Pipeline* pipeline = nullptr;
	vk::PipelineLayout pipelineLayout;

	// descriptorSets to bind
	uint32_t descriptorSetNumber = 0;
	std::vector<vk::DescriptorSet> descriptorSets;
	std::vector<uint32_t> dynamicOffsets;

	// list of functions that will be called during the preparation for StateSet's command buffer recording
	std::vector<std::function<void(StateSet&)>> prepareCallList;

	// list of functions that will be called during StateSet recording into the command buffer;
	// each function is called only if the StateSet is recorded, e.g. only if it contains any drawables or its recording is forced
	std::vector<std::function<void(StateSet&, vk::CommandBuffer)>> recordCallList;

	// parent-child relation
	static const ParentChildListOffsets parentChildListOffsets;
	ChildList<StateSet, parentChildListOffsets> childList;
	ParentList<StateSet, parentChildListOffsets> parentList;

public:

	// construction and destruction
	StateSet(Renderer& renderer) noexcept;
	~StateSet() noexcept;

	// deleted constructors and operators
	StateSet(const StateSet&) = delete;
	StateSet(StateSet&&) = delete;
	StateSet& operator=(const StateSet&) = delete;
	StateSet& operator=(StateSet&&) = delete;

	// getters
	Renderer& renderer() const;
	bool forceRecording() const;

	// rendering methods
	size_t prepareRecording();
	void setForceRecording(bool value);  ///< Sets whether recording of this StateSet will always happen. It means that recordCallLists will be called, allowing the user to record its own draw commands. 
	                                     ///< If set to false, the recording will happen only if there are any Drawables in this StateSet or in any child StateSet. The recording can also be forced by requestRecording() on per-frame basis.
	void requestRecording();  ///< Requests the recording of this StateSet for the current frame even if it does not contain any Drawables. This function shall be called from prepareCallList callbacks only. Otherwise, it has no effect.
	void recordToCommandBuffer(vk::CommandBuffer cb, vk::PipelineLayout currentPipelineLayout, size_t& drawableCounter);

	// drawable methods
	void appendDrawable(Drawable& d, DrawableGpuData gpuData);
	static void removeDrawable(Drawable& d);
	void removeAllDrawables() noexcept;
	Drawable& getDrawable(size_t index) const;
	size_t getNumDrawables() const;

protected:
	void appendDrawableInternal(Drawable& d, DrawableGpuData gpuData);
	void removeDrawableInternal(Drawable& d) noexcept;
	friend Drawable;
};


}

#endif


// inline methods
#if !defined(CADR_STATE_SET_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_STATE_SET_INLINE_FUNCTIONS
namespace CadR {

inline StateSet::StateSet(Renderer& renderer) noexcept : _renderer(&renderer)  {}
inline StateSet::~StateSet() noexcept  { removeAllDrawables(); }
inline Renderer& StateSet::renderer() const  { return *_renderer; }
inline bool StateSet::forceRecording() const  { return _forceRecording; }
inline void StateSet::setForceRecording(bool value)  { _forceRecording = value; }
inline void StateSet::requestRecording()  { _skipRecording = false; }
inline void StateSet::appendDrawable(Drawable& d, DrawableGpuData gpuData)  { if(d._indexIntoStateSet != ~0u) d._stateSet->removeDrawableInternal(d); appendDrawableInternal(d, gpuData); }
inline void StateSet::removeDrawable(Drawable& d)  { if(d._indexIntoStateSet == ~0u) return; d._stateSet->removeDrawableInternal(d); d._indexIntoStateSet=~0u; }
inline Drawable& StateSet::getDrawable(size_t index) const  { return *_drawablePtrList[index]; }
inline size_t StateSet::getNumDrawables() const  { return _drawablePtrList.size(); }

}
#endif
