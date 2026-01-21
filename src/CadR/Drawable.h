#ifndef CADR_DRAWABLE_HEADER
# define CADR_DRAWABLE_HEADER

# ifndef CADR_NO_INLINE_FUNCTIONS
#  define CADR_NO_INLINE_FUNCTIONS
#  include <CadR/DataAllocation.h>
#  include <CadR/MatrixList.h>
#  undef CADR_NO_INLINE_FUNCTIONS
# else
#  include <CadR/DataAllocation.h>
#  include <CadR/MatrixList.h>
# endif
# include <boost/intrusive/list.hpp>

namespace CadR {

class Geometry;
class Renderer;
class StagingData;
class StateSet;


/** DrawableGpuData contains data associated with the Drawable
 *  that are used by GPU during rendering.
 *  It is stored inside StateSet in the host memory
 *  and uploaded to GPU during rendering for all drawables expected to be rendered.
 */
struct DrawableGpuData {
	uint64_t vertexDataHandle;
	uint64_t indexDataHandle;
	uint64_t matrixListHandle;
	uint64_t drawableDataHandle;
	uint64_t primitiveSetHandle;
	uint32_t primitiveSetOffset;
	uint32_t padding;

	DrawableGpuData()  {}
	inline constexpr DrawableGpuData(uint64_t vertexDataHandle, uint64_t indexDataHandle, uint64_t matrixListHandle, uint64_t drawableDataHandle, uint64_t primitiveSetHandle, uint32_t primitiveSetOffset);
};


/** Drawable makes PrimitiveSet rendered.
 *  Drawable is connected with Geometry and one of its PrimitiveSets.
 *  It is also associated with shaderData that are passed to the shaders in the pipeline
 *  to be used during rendering.
 */
class CADR_EXPORT Drawable {
protected:
	StateSet* _stateSet;  ///< StateSet that draws this Drawable. The drawable is drawn if _indexIntoStateSet is valid (any value except ~0). If _indexIntoStateSet is not valid, _stateSet variable keeps the last value that was written there allowing easy re-association with the StateSet when valid value of _indexIntoStateSet is set again.
	MatrixList* _matrixList;
	DataAllocation* _drawableData;
	uint32_t _indexIntoStateSet = ~0;  ///< Index into StateSet data where this Drawable is stored or ~0 if no StateSet is attached. The index might be changing as other Drawables are removed and appended to the StateSet.
	inline Drawable(MatrixList* matrixList, DataAllocation* drawableData);
	void create(Geometry& geometry, uint32_t primitiveSetOffset, MatrixList& matrixList,
	            DataAllocation* drawableData, StateSet& stateSet);
public:
	boost::intrusive::list_member_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink>
	> _drawableListHook;  ///< List hook of Geometry::_drawableList. Geometry::_drawableList contains all Drawables that render the Geometry.
public:

	// construction and destruction
	inline Drawable() noexcept = default;  ///< Constructs empty object.
	Drawable(Geometry& geometry, uint32_t primitiveSetOffset,
	         MatrixList& matrixList, StateSet& stateSet);
	Drawable(Geometry& geometry, uint32_t primitiveSetOffset, MatrixList& matrixList,
	         DataAllocation& drawableData, StateSet& stateSet);
	Drawable(const Drawable&) = delete;  ///< No copy constructor.
	Drawable(Drawable&& other) noexcept;  ///< Move constructor.
	~Drawable() noexcept;

	// operators
	Drawable& operator=(const Drawable&) = delete;  ///< No copy assignment operator.
	Drawable& operator=(Drawable&& rhs) noexcept;  ///< Move assignment operator.

	// create and destroy
	inline void create(Geometry& geometry, uint32_t primitiveSetOffset,
	                   MatrixList& matrixList, StateSet& stateSet);
	inline void create(Geometry& geometry, uint32_t primitiveSetOffset, MatrixList& matrixList,
	                   DataAllocation& drawableData, StateSet& stateSet);
	void destroy() noexcept;
	inline bool isValid() const;

	// getters
	inline Renderer& renderer() const;
	inline StateSet& stateSet() const;
	inline MatrixList& matrixList() const;
	inline DataAllocation* drawableData() const;

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

#endif


// inline functions
#if !defined(CADR_DRAWABLE_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_DRAWABLE_INLINE_FUNCTIONS
# include <CadR/Renderer.h>
# include <CadR/StateSet.h>
namespace CadR {

inline constexpr DrawableGpuData::DrawableGpuData(uint64_t vertexDataHandle_, uint64_t indexDataHandle_, uint64_t matrixListHandle_, uint64_t drawableDataHandle_, uint64_t primitiveSetHandle_, uint32_t primitiveSetOffset_)
	: vertexDataHandle(vertexDataHandle_), indexDataHandle(indexDataHandle_), matrixListHandle(matrixListHandle_), drawableDataHandle(drawableDataHandle_), primitiveSetHandle(primitiveSetHandle_), primitiveSetOffset(primitiveSetOffset_), padding(0) {}

inline Drawable::Drawable(MatrixList* matrixList, DataAllocation* drawableData)  : _matrixList(matrixList), _drawableData(drawableData) {}
inline bool Drawable::isValid() const  { return _indexIntoStateSet!=~0u; }
inline void Drawable::create(Geometry& geometry, uint32_t primitiveSetOffset, MatrixList& matrixList, StateSet& stateSet)  { create(geometry, primitiveSetOffset, matrixList, nullptr, stateSet); }
inline void Drawable::create(Geometry& geometry, uint32_t primitiveSetOffset, MatrixList& matrixList, DataAllocation& drawableData, StateSet& stateSet)  { create(geometry, primitiveSetOffset, matrixList, &drawableData, stateSet); }

inline Renderer& Drawable::renderer() const  { return _stateSet->renderer(); }
inline StateSet& Drawable::stateSet() const  { return *_stateSet; }
inline MatrixList& Drawable::matrixList() const  { return *_matrixList; }
inline DataAllocation* Drawable::drawableData() const  { return _drawableData; }

}
#endif
