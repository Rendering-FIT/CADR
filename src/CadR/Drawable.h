#pragma once

#include <CadR/DataAllocation.h>
#include <boost/intrusive/list.hpp>

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
	constexpr DrawableGpuData(uint64_t vertexDataHandle, uint64_t indexDataHandle, uint64_t primitiveSetHandle, uint64_t shaderDataHandle, uint32_t primitiveSetOffset, uint32_t numInstances);
};


/** Drawable makes PrimitiveSet rendered.
 *  Drawable is connected with Geometry and one of its PrimitiveSets.
 *  It is also associated with shaderData that are passed to the shaders in the pipeline
 *  to be used during rendering.
 */
class CADR_EXPORT Drawable {
protected:
	StateSet* _stateSet;  ///< StateSet that draws this Drawable. The drawable is drawn if _indexIntoStateSet is valid (any value except ~0). If _indexIntoStateSet is not valid, _stateSet variable keeps the last value that was written there allowing easy re-association with the StateSet when valid value of _indexIntoStateSet is set again.
	DataAllocation _shaderData = DataAllocation(nullptr);
	uint32_t _indexIntoStateSet = ~0;  ///< Index into StateSet data where this Drawable is stored or ~0 if no StateSet is attached. The index might be changing as other Drawables are removed and appended to the StateSet.
public:
	boost::intrusive::list_member_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink>
	> _drawableListHook;  ///< List hook of Geometry::_drawableList. Geometry::_drawableList contains all Drawables that render the Geometry.
public:

	// construction and destruction
	Drawable(Renderer& r);  ///< Constructs empty object.
	Drawable(Geometry& geometry, uint32_t primitiveSetOffset, uint32_t numInstances, StateSet& stateSet);
	Drawable(Geometry& geometry, uint32_t primitiveSetOffset, StagingData& shaderStagingData,
	         size_t shaderDataSize, uint32_t numInstances, StateSet& stateSet);
	Drawable(const Drawable&) = delete;  ///< No copy constructor.
	Drawable(Drawable&& other) noexcept;  ///< Move constructor.
	~Drawable() noexcept;

	// operators
	Drawable& operator=(const Drawable&) = delete;  ///< No copy assignment operator.
	Drawable& operator=(Drawable&& rhs) noexcept;  ///< Move assignment operator.

	StagingData create(Geometry& geometry, uint32_t primitiveSetOffset,
	                   size_t shaderDataSize, uint32_t numInstances, StateSet& stateSet);
	void create(Geometry& geometry, uint32_t primitiveSetOffset, uint32_t numInstances, StateSet& stateSet);
	void destroy() noexcept;
	bool isValid() const;

	StagingData reallocShaderData(size_t size, uint32_t numInstances);
	StagingData reallocShaderData(size_t size);
	StagingData createStagingShaderData();
	StagingData createStagingShaderData(size_t size);
	void uploadShaderData(void* data, size_t size);
	void freeShaderData();

	Renderer& renderer() const;
	StateSet& stateSet() const;
	DataAllocation& shaderData();
	const DataAllocation& shaderData() const;
	size_t shaderDataSize() const;

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
#include <CadR/DataAllocation.h>
namespace CadR {

inline constexpr DrawableGpuData::DrawableGpuData(uint64_t vertexDataHandle_, uint64_t indexDataHandle_, uint64_t primitiveSetHandle_, uint64_t shaderDataHandle_, uint32_t primitiveSetOffset_, uint32_t numInstances_)
	: vertexDataHandle(vertexDataHandle_), indexDataHandle(indexDataHandle_), primitiveSetHandle(primitiveSetHandle_), shaderDataHandle(shaderDataHandle_), primitiveSetOffset(primitiveSetOffset_), numInstances(numInstances_)  {}

inline Drawable::Drawable(Renderer& r) : _shaderData(r.dataStorage(), DataAllocation::noHandle)  {}
inline bool Drawable::isValid() const  { return _indexIntoStateSet!=~0u; }
inline void Drawable::uploadShaderData(void* data, size_t size)  { _shaderData.upload(data, size); }
inline StagingData Drawable::createStagingShaderData()  { return _shaderData.createStagingData(); }
inline StagingData Drawable::createStagingShaderData(size_t size)  { return _shaderData.createStagingData(size); }

inline StateSet& Drawable::stateSet() const  { return *_stateSet; }
inline DataAllocation& Drawable::shaderData()  { return _shaderData; }
inline const DataAllocation& Drawable::shaderData() const  { return _shaderData; }
inline size_t Drawable::shaderDataSize() const  { return _shaderData.size(); }

}

// include inline Drawable functions defined in StateSet.h (they are defined there because they depend on CadR::StateSet class)
#include <CadR/StateSet.h>
