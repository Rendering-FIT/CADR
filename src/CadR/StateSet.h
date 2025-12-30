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
# include <boost/intrusive/list.hpp>
# include <functional>

namespace CadR {

class Pipeline;
class Renderer;
class Texture;


struct StateSetDescriptorUpdater {
	std::function<void(Texture& t)> updateFunc;
	vk::DescriptorSet descritorSet;

	boost::intrusive::list_member_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink>
	> _stateSetHook;  ///< List hook of StateSet::_descriptorUpdaterList.
	boost::intrusive::list_member_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink>
	> _updaterHook;  ///< List hook of the object doing descriptor update.
};


class CADR_EXPORT StateSet {
protected:

	Renderer* _renderer;  ///< Renderer associated with this Stateset.
	bool _skipRecording;  ///< The internal flag optimizing the rendering. When set, it excludes this and all the child StateSets from recording into the command buffer.
	                      ///< By default, it is set to true whenever there are no Drawables in this StateSet and in any child StateSets. It can be forced to false by calling setForceRecording() or requestRecording().
	bool _forceRecording = false;  ///< The flag forces recording to be performed always, even if the StateSet does not contain any Drawables, neither its children. This might be useful as some draw commands might be recorded by the user in the callbacks. 
	std::vector<DrawableGpuData> _drawableDataList;  ///< List of Drawable data that is sent to GPU when Drawables are rendered.
	std::vector<Drawable*> _drawablePtrList;  ///< List of Drawables attached to this StateSet.
	vk::DescriptorPool _descriptorPool;
	std::vector<vk::DescriptorSet> _descriptorSetList;
	uint32_t _firstDescriptorSetIndex = 0;
	std::vector<uint32_t> _dynamicOffsets;

	boost::intrusive::list<
		StateSetDescriptorUpdater,
		boost::intrusive::member_hook<
			StateSetDescriptorUpdater,
			boost::intrusive::list_member_hook<
				boost::intrusive::link_mode<boost::intrusive::auto_unlink>>,
			&StateSetDescriptorUpdater::_stateSetHook>,
		boost::intrusive::constant_time_size<false>
	> _descriptorUpdaterList;

public:

	// pipeline to bind
	const CadR::Pipeline* pipeline = nullptr;

	// list of functions that will be called during the preparation for StateSet's command buffer recording
	std::vector<std::function<void(StateSet&)>> prepareCallList;

	// list of functions that will be called during StateSet recording into the command buffer;
	// each function is called only if the StateSet is recorded, e.g. only if it contains any drawables or its recording is forced
	std::vector<std::function<void(StateSet&, vk::CommandBuffer, vk::PipelineLayout)>> recordCallList;

	// parent-child relation
	static const ParentChildListOffsets parentChildListOffsets;
	ChildList<StateSet, parentChildListOffsets> childList;
	ParentList<StateSet, parentChildListOffsets> parentList;

public:

	// construction and destruction
	inline StateSet(Renderer& renderer) noexcept;
	inline ~StateSet() noexcept;
	inline void destroy() noexcept;

	// deleted constructors and operators
	StateSet(const StateSet&) = delete;
	StateSet(StateSet&&) = delete;
	StateSet& operator=(const StateSet&) = delete;
	StateSet& operator=(StateSet&&) = delete;

	// getters
	inline Renderer& renderer() const;
	inline bool forceRecording() const;
	inline vk::DescriptorPool descriptorPool() const;
	inline const std::vector<vk::DescriptorSet>& descriptorSetList() const;
	inline vk::DescriptorSet descriptorSet(size_t index) const;
	inline uint32_t firstDescriptorSetIndex() const;
	inline const std::vector<uint32_t>& dynamicOffsets() const;

	// descriptorSet functions
	void allocDescriptorSet(vk::DescriptorType type, vk::DescriptorSetLayout layout);  //< Allocate one DescriptorSet if layout contains single descriptor. Type of the descriptor must be passed in type parameter. All previously allocated DescriptorSets are freed.
	void allocDescriptorSet(uint32_t poolSizeCount, vk::DescriptorPoolSize* poolSizeList, vk::DescriptorSetLayout layout);  //< Allocate one DestriptorSet whose destriptors are described in poolSizeList. The descriptors must be in accordance to the layout. All previously allocated DescriptorSets are freed.
	void allocDescriptorSets(const vk::DescriptorPoolCreateInfo& descriptorPoolCreateInfo,
		const std::vector<vk::DescriptorSetLayout>& layoutList, const void* descriptorInfoPNext = nullptr);  //< Allocate one or more DescriptorSets, according to descriptorPoolCreateInfo parameter. Layout of each DescriptorSet is given in layoutList parameter. All previously allocated DescriptorSets are freed.
	void allocDescriptorSets(const vk::DescriptorPoolCreateInfo& descriptorPoolCreateInfo,
		uint32_t layoutCount, const vk::DescriptorSetLayout* layoutList, const void* descriptorInfoPNext = nullptr);  //< Allocate one or more DescriptorSets, according to descriptorPoolCreateInfo parameter. Layout of each DescriptorSet is given in layoutList parameter. All previously allocated DescriptorSets are freed.
	void freeDescriptorSets() noexcept;  //< Releases all DescriptorSets allocated for the StateSet.
	inline void setDescriptorSets(vk::DescriptorPool descriptorPool, std::vector<vk::DescriptorSet>&& descriptorSetList);  //< Set descriptor pool and descriptor set list and pass their ownership to this StateSet.
	void updateDescriptorSet(const vk::WriteDescriptorSet& w);  //< Perform single descriptor set update, as specified by the parameter w.
	void updateDescriptorSet(uint32_t writeCount, vk::WriteDescriptorSet* writes);  //< Perform number of DescriptorSet updates, as described by writes parameter.
	void updateDescriptorSet(uint32_t writeCount, vk::WriteDescriptorSet* writes, uint32_t copyCount, vk::CopyDescriptorSet* copies);  //< Perform DescriptorSet updates and copies.
	inline void setFirstDescriptorSetIndex(uint32_t value);
	inline void setDynamicOffsets(const std::vector<uint32_t>& offsets);
	inline void setDynamicOffsets(std::vector<uint32_t>&& offsets);
	inline StateSetDescriptorUpdater& createDescriptorUpdater(
		std::function<void(CadR::Texture& t)>&& updateFunc,
		vk::DescriptorSet descriptorSet);

	// rendering functions
	size_t prepareRecording();
	inline void setForceRecording(bool value);  ///< Sets whether recording of this StateSet will always happen. It means that recordCallLists will be called, allowing the user to record its own draw commands.
		///< If set to false, the recording will happen only if there are any Drawables in this StateSet or in any child StateSet. The recording can also be forced by requestRecording() on per-frame basis.
	inline void requestRecording();  ///< Requests the recording of this StateSet for the current frame even if it does not contain any Drawables. This function shall be called from prepareCallList callbacks only. Otherwise, it has no effect.
	void recordToCommandBuffer(vk::CommandBuffer cb, vk::PipelineLayout currentPipelineLayout, size_t& drawableCounter);

	// drawable functions
	inline void appendDrawable(Drawable& d, DrawableGpuData gpuData);
	static inline void removeDrawable(Drawable& d);
	void removeAllDrawables() noexcept;
	inline Drawable& getDrawable(size_t index) const;
	inline size_t getNumDrawables() const;

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
inline StateSet::~StateSet() noexcept  { destroy(); }
inline void StateSet::destroy() noexcept  { _descriptorUpdaterList.clear_and_dispose([](StateSetDescriptorUpdater* u){ delete u; }); freeDescriptorSets(); removeAllDrawables(); }
inline Renderer& StateSet::renderer() const  { return *_renderer; }
inline bool StateSet::forceRecording() const  { return _forceRecording; }
inline vk::DescriptorPool StateSet::descriptorPool() const  { return _descriptorPool; }
inline const std::vector<vk::DescriptorSet>& StateSet::descriptorSetList() const  { return _descriptorSetList; }
inline vk::DescriptorSet StateSet::descriptorSet(size_t index) const  { return _descriptorSetList[index]; }
inline uint32_t StateSet::firstDescriptorSetIndex() const  { return _firstDescriptorSetIndex; }
inline const std::vector<uint32_t>& StateSet::dynamicOffsets() const  { return _dynamicOffsets; }
inline void StateSet::allocDescriptorSets(const vk::DescriptorPoolCreateInfo& descriptorPoolCreateInfo, const std::vector<vk::DescriptorSetLayout>& layoutList, const void* descriptorInfoPNext)  { allocDescriptorSets(descriptorPoolCreateInfo, uint32_t(layoutList.size()), layoutList.data(), descriptorInfoPNext); }
inline void StateSet::setDescriptorSets(vk::DescriptorPool descriptorPool, std::vector<vk::DescriptorSet>&& descriptorSetList)  { freeDescriptorSets(); _descriptorPool = descriptorPool; _descriptorSetList = std::move(descriptorSetList); }
inline void StateSet::setFirstDescriptorSetIndex(uint32_t value)  { _firstDescriptorSetIndex = value; }
inline void StateSet::setDynamicOffsets(const std::vector<uint32_t>& offsets)  { _dynamicOffsets = offsets; }
inline void StateSet::setDynamicOffsets(std::vector<uint32_t>&& offsets)  { _dynamicOffsets = std::move(offsets); }
inline StateSetDescriptorUpdater& StateSet::createDescriptorUpdater(std::function<void(Texture& t)>&& updateFunc, vk::DescriptorSet descriptorSet)  { auto& updater=*new StateSetDescriptorUpdater{ std::move(updateFunc), descriptorSet }; _descriptorUpdaterList.push_back(updater); return updater; }
inline void StateSet::setForceRecording(bool value)  { _forceRecording = value; }
inline void StateSet::requestRecording()  { _skipRecording = false; }
inline void StateSet::appendDrawable(Drawable& d, DrawableGpuData gpuData)  { if(d._indexIntoStateSet != ~0u) d._stateSet->removeDrawableInternal(d); appendDrawableInternal(d, gpuData); }
inline void StateSet::removeDrawable(Drawable& d)  { if(d._indexIntoStateSet == ~0u) return; d._stateSet->removeDrawableInternal(d); d._indexIntoStateSet=~0u; }
inline Drawable& StateSet::getDrawable(size_t index) const  { return *_drawablePtrList[index]; }
inline size_t StateSet::getNumDrawables() const  { return _drawablePtrList.size(); }

}
#endif
