#ifndef CADR_DRAWABLE_HEADER
# define CADR_DRAWABLE_HEADER

# ifndef CADR_NO_INLINE_FUNCTIONS
#  define CADR_NO_INLINE_FUNCTIONS
#  include <CadR/DataAllocation.h>
#  undef CADR_NO_INLINE_FUNCTIONS
# else
#  include <CadR/DataAllocation.h>
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
	uint64_t primitiveSetHandle;
	uint64_t shaderDataHandle;
	uint32_t primitiveSetOffset;
	uint32_t numInstances;

	DrawableGpuData()  {}
	inline constexpr DrawableGpuData(uint64_t vertexDataHandle, uint64_t indexDataHandle, uint64_t primitiveSetHandle, uint64_t shaderDataHandle, uint32_t primitiveSetOffset, uint32_t numInstances);
};


/** Drawable makes PrimitiveSet rendered.
 *  Drawable is connected with Geometry and one of its PrimitiveSets.
 *  It is also associated with shaderData that are passed to the shaders in the pipeline
 *  to be used during rendering.
 */
class CADR_EXPORT Drawable {
protected:
	StateSet* _stateSet;  ///< StateSet that draws this Drawable. The drawable is drawn if _indexIntoStateSet is valid (any value except ~0). If _indexIntoStateSet is not valid, _stateSet variable keeps the last value that was written there allowing easy re-association with the StateSet when valid value of _indexIntoStateSet is set again.
	DataAllocation _shaderData;
	uint32_t _indexIntoStateSet = ~0;  ///< Index into StateSet data where this Drawable is stored or ~0 if no StateSet is attached. The index might be changing as other Drawables are removed and appended to the StateSet.
public:
	boost::intrusive::list_member_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink>
	> _drawableListHook;  ///< List hook of Geometry::_drawableList. Geometry::_drawableList contains all Drawables that render the Geometry.
public:

	// construction and destruction
	inline Drawable(Renderer& r) noexcept;  ///< Constructs empty object.
	Drawable(Geometry& geometry, uint32_t primitiveSetOffset, uint32_t numInstances, StateSet& stateSet);
	Drawable(Geometry& geometry, uint32_t primitiveSetOffset, StagingData& shaderStagingData,
	         size_t shaderDataSize, uint32_t numInstances, StateSet& stateSet);
	Drawable(const Drawable&) = delete;  ///< No copy constructor.
	Drawable(Drawable&& other) noexcept;  ///< Move constructor.
	~Drawable() noexcept;

	// operators
	Drawable& operator=(const Drawable&) = delete;  ///< No copy assignment operator.
	Drawable& operator=(Drawable&& rhs) noexcept;  ///< Move assignment operator.

	// create and destroy
	StagingData create(Geometry& geometry, uint32_t primitiveSetOffset,
	                   size_t shaderDataSize, uint32_t numInstances, StateSet& stateSet);
	void create(Geometry& geometry, uint32_t primitiveSetOffset, uint32_t numInstances, StateSet& stateSet);
	void destroy() noexcept;
	inline bool isValid() const;

	// shader data and staging data
	StagingData reallocShaderData(size_t size, uint32_t numInstances);
	StagingData reallocShaderData(size_t size);
	inline StagingData createStagingShaderData();
	inline StagingData createStagingShaderData(size_t size);
	inline void uploadShaderData(void* data, size_t size);
	void freeShaderData();

	// getters
	inline Renderer& renderer() const;
	inline StateSet& stateSet() const;
	inline DataAllocation& shaderData();
	inline const DataAllocation& shaderData() const;
	inline size_t shaderDataSize() const;

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


// inline methods
#if !defined(CADR_DRAWABLE_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_DRAWABLE_INLINE_FUNCTIONS
# include <CadR/Renderer.h>
# include <CadR/StateSet.h>
namespace CadR {

inline constexpr DrawableGpuData::DrawableGpuData(uint64_t vertexDataHandle_, uint64_t indexDataHandle_, uint64_t primitiveSetHandle_, uint64_t shaderDataHandle_, uint32_t primitiveSetOffset_, uint32_t numInstances_)
	: vertexDataHandle(vertexDataHandle_), indexDataHandle(indexDataHandle_), primitiveSetHandle(primitiveSetHandle_), shaderDataHandle(shaderDataHandle_), primitiveSetOffset(primitiveSetOffset_), numInstances(numInstances_)  {}

inline Drawable::Drawable(Renderer& r) noexcept : _shaderData(r.dataStorage(), DataAllocation::noHandle)  {}
inline bool Drawable::isValid() const  { return _indexIntoStateSet!=~0u; }
inline StagingData Drawable::createStagingShaderData()  { return _shaderData.createStagingData(); }
inline StagingData Drawable::createStagingShaderData(size_t size)  { return _shaderData.createStagingData(size); }
inline void Drawable::uploadShaderData(void* data, size_t size)  { _shaderData.upload(data, size); }

inline Renderer& Drawable::renderer() const  { return _stateSet->renderer(); }
inline StateSet& Drawable::stateSet() const  { return *_stateSet; }
inline DataAllocation& Drawable::shaderData()  { return _shaderData; }
inline const DataAllocation& Drawable::shaderData() const  { return _shaderData; }
inline size_t Drawable::shaderDataSize() const  { return _shaderData.size(); }

}
#endif
