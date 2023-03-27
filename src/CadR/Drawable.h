#pragma once

#include <boost/intrusive/list.hpp>

namespace CadR {

class DataAllocation;
class Geometry;
class Renderer;
class StagingData;
class StateSet;
struct StateSetDrawableContainer;


/** DrawableGpuData contains data associated with the Drawable
 *  that are used by GPU during rendering.
 *  It is stored inside StateSet in the host memory
 *  and uploaded to GPU during rendering for all drawables expected to be rendered.
 */
struct DrawableGpuData {
	uint64_t primitiveSetAddr;
	uint64_t shaderDataAddr;
	uint32_t numInstances;
	uint32_t dummy;

	DrawableGpuData()  {}
	constexpr DrawableGpuData(uint64_t primitiveSetAddr, uint64_t shaderDataAddr, uint32_t numInstances);
};


/** Drawable makes PrimitiveSet rendered.
 *  Drawable is connected with Geometry and one of its PrimitiveSets.
 *  It is also associated with shaderData that are passed to the shaders in the pipeline
 *  to be used during rendering.
 */
class CADR_EXPORT Drawable {
protected:
	StateSetDrawableContainer* _stateSetDrawableContainer;  ///< StateSet that draws this Drawable. The drawable is drawn if _indexIntoStateSet is valid (any value except ~0). If _indexIntoStateSet is not valid, _stateSet variable keeps the last value that was written there allowing easy re-association with the StateSet when valid value of _indexIntoStateSet is set again.
	DataAllocation* _shaderData = nullptr;
	uint32_t _indexIntoStateSet = ~0;  ///< Index to StateSet data where this Drawable is stored or ~0 if no StateSet attached. The index might be changing as other Drawables are removed and appended to the StateSet.
public:
	boost::intrusive::list_member_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink>
	> _drawableListHook;  ///< List hook of Geometry::_drawableList. Geometry::_drawableList contains all Drawables that render the Geometry.
public:

	Drawable() noexcept = default;  ///< Default constructor. It leaves object largely uninitialized. It is dangerous to call many functions of this object until you initialize it by calling create().
	Drawable(Geometry& geometry, uint32_t primitiveSetIndex, size_t shaderDataSize, uint32_t numInstances, StateSet& stateSet);
	Drawable(Drawable&& other) noexcept;
	Drawable& operator=(Drawable&& rhs) noexcept;
	~Drawable() noexcept;

	Drawable(const Drawable&) = delete;
	Drawable& operator=(const Drawable&) = delete;

	void create(Geometry& geometry, uint32_t primitiveSetIndex, size_t shaderDataSize, uint32_t numInstances, StateSet& stateSet);
	//void create(Geometry& geometry, uint32_t primitiveSetIndex);
	void allocShaderData(size_t size, uint32_t numInstances);
	void allocShaderData(size_t size);
	void freeShaderData();
	void destroy() noexcept;
	bool isValid() const;

	void uploadShaderData(void* data, size_t size);
	StagingData createStagingData();

	Renderer* renderer() const;
	StateSet* stateSet() const;
	DataAllocation* shaderData() const;
	size_t shaderDataSize() const;
	//uint32_t numInstances() const;

	friend StateSet;
	friend StateSetDrawableContainer;
	friend class DataMemory;
protected:
	static void moveCallback(DataAllocation* oldAlloc, DataAllocation* newAlloc, void* userData);
};


typedef boost::intrusive::list<
	Drawable,
	boost::intrusive::member_hook<
		Drawable,
		boost::intrusive::list_member_hook<
			boost::intrusive::link_mode<boost::intrusive::auto_unlink>>,
		&Drawable::_drawableListHook>,
	boost::intrusive::constant_time_size<false>
> DrawableList;


}


// inline methods
#include <CadR/DataAllocation.h>
#include <CadR/StateSet.h>
namespace CadR {

inline constexpr DrawableGpuData::DrawableGpuData(uint64_t primitiveSetAddr_, uint64_t shaderDataAddr_, uint32_t numInstances_)
	: primitiveSetAddr(primitiveSetAddr_), shaderDataAddr(shaderDataAddr_), numInstances(numInstances_), dummy(0)  {}

inline bool Drawable::isValid() const  { return _indexIntoStateSet!=~0u; }

inline Renderer* Drawable::renderer() const  { return _stateSetDrawableContainer->stateSet->renderer(); }
inline StateSet* Drawable::stateSet() const  { return _stateSetDrawableContainer->stateSet; }
inline DataAllocation* Drawable::shaderData() const  { return _shaderData; }
inline size_t Drawable::shaderDataSize() const  { return _shaderData ? _shaderData->size() : 0; }

}
