#pragma once

#include <CadR/AllocationManagers.h>
#include <boost/intrusive/list.hpp>

namespace CadR {

class Geometry;
class Renderer;
class StagingBuffer;
class StateSet;
struct StateSetDrawableContainer;


/** DrawableGpuData contains data associated with the Drawable
 *  that are used by GPU during rendering.
 *  It is stored inside StateSet in the host memory
 *  and uploaded to GPU during rendering for all drawables expected to be rendered.
 */
struct DrawableGpuData {
	uint64_t primitiveSetAddr;
	uint32_t shaderDataOffset4;
	uint32_t dummy;

	DrawableGpuData()  {}
	constexpr DrawableGpuData(uint64_t primitiveSetAddr, uint32_t shaderDataOffset4);
};


/** Drawable makes PrimitiveSet rendered.
 *  Drawable is connected with Geometry and one of its PrimitiveSets.
 *  It is also associated with shaderData that are passed to the shaders in the pipeline
 *  to be used during rendering.
 */
class CADR_EXPORT Drawable {
protected:
	StateSetDrawableContainer* _stateSetDrawableContainer;  ///< StateSet that draws this Drawable. The drawable is drawn if _indexIntoStateSet is valid (any value except ~0). If _indexIntoStateSet is not valid, _stateSet variable keeps the last value that was written there allowing easy re-association with the StateSet when valid value of _indexIntoStateSet is set again.
	uint32_t _indexIntoStateSet;  ///< Index to StateSet data where this Drawable is stored or ~0 if no StateSet attached. The index might be changing as other Drawables are removed and appended to the StateSet.
	uint32_t _shaderDataID;  ///< ShaderData that will be passed to the shaders during rendering.
public:
	boost::intrusive::list_member_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink>
	> _drawableListHook;  ///< List hook of Geometry::_drawableList. Geometry::_drawableList contains all Drawables that render the Geometry.
public:

	Drawable();  ///< Default constructor. It leaves object largely uninitialized. It is dangerous to call many of its functions until you initialize it by calling create().
	Drawable(Geometry& geometry,uint32_t primitiveSetIndex,uint32_t shaderDataID,StateSet& stateSet);
	Drawable(Drawable&& other);
	Drawable& operator=(Drawable&& rhs);
	~Drawable();

	Drawable(const Drawable&) = delete;
	Drawable& operator=(const Drawable&) = delete;

	void create(Geometry& geometry,uint32_t primitiveSetIndex,uint32_t shaderDataID,StateSet& stateSet);
	void create(Geometry& geometry,uint32_t primitiveSetIndex);
	void destroy();
	bool isValid() const;

	Renderer* renderer() const;
	StateSet* stateSet() const;
	uint32_t shaderDataID() const;

	friend StateSet;
	friend StateSetDrawableContainer;
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
#include <CadR/StateSet.h>
namespace CadR {

inline constexpr DrawableGpuData::DrawableGpuData(uint64_t primitiveSetAddr_, uint32_t shaderDataOffset4_)
	: primitiveSetAddr(primitiveSetAddr_), shaderDataOffset4(shaderDataOffset4_), dummy(0)  {}

inline Drawable::Drawable() : _indexIntoStateSet(~0)  {}
inline Drawable::~Drawable()  { if(_indexIntoStateSet!=~0u) _stateSetDrawableContainer->removeDrawableUnsafe(*this); }
inline void Drawable::destroy()  { if(_indexIntoStateSet==~0u) return; _stateSetDrawableContainer->removeDrawableUnsafe(*this); _drawableListHook.unlink(); _indexIntoStateSet=~0u; }
inline bool Drawable::isValid() const  { return _indexIntoStateSet!=~0u; }

inline Renderer* Drawable::renderer() const  { return _stateSetDrawableContainer->stateSet->renderer(); }
inline StateSet* Drawable::stateSet() const  { return _stateSetDrawableContainer->stateSet; }
inline uint32_t Drawable::shaderDataID() const  { return _shaderDataID; }

}
