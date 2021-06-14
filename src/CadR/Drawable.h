#pragma once

#include <boost/intrusive/list.hpp>

namespace CadR {

class Geometry;
class Renderer;
class StateSet;


/** DrawableGpuData contains Drawable's data that are used by GPU for rendering.
 *  It is stored inside StateSet in the host memory and uploaded to GPU during rendering.
 */
struct DrawableGpuData final {
	uint32_t primitiveSetOffset4;
	uint32_t shaderDataOffset4;

	DrawableGpuData()  {}
	constexpr DrawableGpuData(uint32_t primitiveSetOffset4, uint32_t shaderDataOffset4);
};


/** Drawable makes PrimitiveSet rendered.
 *  Drawable is connected with Geometry and one of its PrimitiveSets.
 *  It is also associated with shaderData that are passed to the shaders in the pipeline
 *  during rendering.
 */
class CADR_EXPORT Drawable final {
protected:
	StateSet* _stateSet;  ///< StateSet that renders this Drawable. The StateSet can render the Drawable if _indexIntoStateSet is valid (any value except ~0). If _indexIntoStateSet is not valid, _stateSet keeps the last value that was written there. This allows keeping Drawable association with the StateSet until _indexIntoStateSet gets valid value again.
	uint32_t _indexIntoStateSet;  ///< Index to StateSet data where this Drawable is stored or ~0 if no StateSet is attached. The index is not constant over time but the value might be changing as other Drawables are removed and appended to the StateSet.
	uint32_t _shaderDataID;  ///< ShaderData that will be passed to shaders during rendering.
public:
	boost::intrusive::list_member_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink>
	> _drawableListHook;  ///< List hook of Geometry::_drawableList. Geometry::_drawableList contains all Drawables that are associated with the Geometry.
public:

	Drawable();  ///< Default constructor. It leaves object largely uninitialized. It is dangerous to call many of Drawable's functions until you initialize it by calling create().
	Drawable(uint32_t shaderDataID, StateSet& stateSet);  ///< Constructs Drawable and assigns shaderDataID and StateSet, but does not create internal rendering object. The Drawable will not participate in the rendering process until create() is called on it.
	Drawable(Geometry& geometry, uint32_t primitiveSetIndex, uint32_t shaderDataID, StateSet& stateSet);  ///< Constructs Drawable and its internal rendering object. The Drawable participates in the rendering process.
	Drawable(Drawable&& other);  ///< Move constructor.
	Drawable& operator=(Drawable&& rhs);  ///< Move operator.
	~Drawable();  ///< Destructor.

	Drawable(const Drawable&) = delete;
	Drawable& operator=(const Drawable&) = delete;

	void create(Geometry& geometry, uint32_t primitiveSetIndex, uint32_t shaderDataID, StateSet& stateSet);  ///< It creates the internal rendering object that causes the Drawable to participate in the rendering process. If internal rendering object already exists, it is updated according to the given parameters.
	void create(Geometry& geometry, uint32_t primitiveSetIndex);  ///< It creates the internal rendering object. StateSet and shaderDataID needs to be set by some previously called method or constructor. The Drawable will participate in the rendering process now. IF internal rendering object already exists, it is updated according to the given parameters.
	void destroy();  ///< Destroys the internal rendering object. From now on, the Drawable will not participate in the rendering process. StateSet and shaderDataID are preserved inside the Drawable and might be reused by create().
	bool isValid() const;  ///< Returns true if internal rendering object exists and, as the effect, the Drawable participates in the rendering process.

	Renderer* renderer() const;  ///< Returns Renderer.
	StateSet* stateSet() const;  ///< Returns StateSet.
	uint32_t shaderDataID() const;  ///< Returns shaderDataID.

	friend StateSet;
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

inline constexpr DrawableGpuData::DrawableGpuData(uint32_t primitiveSetOffset4_, uint32_t shaderDataOffset4_)
	: primitiveSetOffset4(primitiveSetOffset4_), shaderDataOffset4(shaderDataOffset4_)  {}
inline Drawable::Drawable() : _indexIntoStateSet(~0)  {}
inline Drawable::Drawable(uint32_t shaderDataID, StateSet& stateSet) : _stateSet(&stateSet), _indexIntoStateSet(~0), _shaderDataID(shaderDataID)  {}
inline Drawable::~Drawable()  { if(_indexIntoStateSet!=~0u) _stateSet->removeDrawableUnsafe(*this); _drawableListHook.unlink(); }
inline void Drawable::destroy()  { _stateSet->removeDrawable(*this); _drawableListHook.unlink(); }
inline bool Drawable::isValid() const  { return _indexIntoStateSet!=~0u; }

inline Renderer* Drawable::renderer() const  { return _stateSet->renderer(); }
inline StateSet* Drawable::stateSet() const  { return _stateSet; }
inline uint32_t Drawable::shaderDataID() const  { return _shaderDataID; }

}
