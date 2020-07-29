#pragma once

#include <CadR/AllocationManagers.h>
#include <boost/intrusive/list.hpp>

namespace CadR {

class Geometry;
class Renderer;
class StagingBuffer;
class StateSet;


/** DrawableGpuData contains data associated with Drawable that are stored in GPU buffers,
 *  usually in Renderer::drawCommandBuffer().
 *  Allocation management of DrawableGpuData buffer is usually provided
 *  by Renderer::drawableAllocationManager().
 */
struct DrawableGpuData final {
	uint32_t primitiveSetOffset4;
	uint32_t shaderDataOffset4;
	uint32_t stateSetOffset4;
	uint32_t userData;  ///< Provides 16-byte structure alignment.

	DrawableGpuData()  {}
	DrawableGpuData(uint32_t primitiveSetOffset4,uint32_t shaderDataOffset4,uint32_t stateSetOffset4);
	constexpr DrawableGpuData(uint32_t primitiveSetOffset4,uint32_t shaderDataOffset4,
	                          uint32_t stateSetOffset4,uint32_t userData);
};


class CADR_EXPORT Drawable final {
protected:
	ItemAllocation _gpuAllocation;
	uint32_t _shaderDataID;
	StateSet* _stateSet;
public:
	boost::intrusive::list_member_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink>
	> _drawableListHook;  ///< List hook of Geometry::_drawableList. Geometry::_drawableList contains all Drawables that render the Geometry.
public:

	Drawable() = default;  ///< Default constructor. It leaves object largely uninitialized. It is dangerous to call many of its functions until you initialize it by calling create().
	Drawable(uint32_t shaderDataID,StateSet* stateSet);
	Drawable(Geometry* geometry,uint32_t primitiveSetIndex,uint32_t shaderDataID,StateSet* stateSet,uint32_t userData=0);
	Drawable(Drawable&& other);
	Drawable& operator=(Drawable&& rhs);
	~Drawable();

	Drawable(const Drawable&) = delete;
	Drawable& operator=(const Drawable&) = delete;

	void create(Geometry* geometry,uint32_t primitiveSetIndex,uint32_t shaderDataID,StateSet* stateSet,uint32_t userData=0);
	void create(Geometry* geometry,uint32_t primitiveSetIndex,uint32_t userData=0);
	void destroy();
	bool isValid() const;

	Renderer* renderer() const;
	const ItemAllocation& allocation() const;
	uint32_t shaderDataID() const;
	StateSet* stateSet() const;

	void upload(const DrawableGpuData& drawableData);
	StagingBuffer createStagingBuffer();

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
#include <CadR/Renderer.h>
#include <CadR/StagingBuffer.h>
#include <CadR/StateSet.h>
namespace CadR {

inline constexpr DrawableGpuData::DrawableGpuData(uint32_t primitiveSetOffset4_,uint32_t shaderDataOffset4_,uint32_t stateSetOffset4_,uint32_t userData_)
	: primitiveSetOffset4(primitiveSetOffset4_), shaderDataOffset4(shaderDataOffset4_), stateSetOffset4(stateSetOffset4_), userData(userData_)  {}
inline Drawable::Drawable(uint32_t shaderDataID,StateSet* stateSet) : _shaderDataID(shaderDataID), _stateSet(stateSet)  {}
inline Drawable::~Drawable()  { destroy(); }
inline void Drawable::destroy()
{
	if(!_gpuAllocation.isValid())
		return;
	_gpuAllocation.free(renderer()->drawableAllocationManager());
	_drawableListHook.unlink();
	_stateSet->decrementNumDrawables();
}
inline bool Drawable::isValid() const  { return _gpuAllocation.isValid(); }

inline Renderer* Drawable::renderer() const  { return _stateSet->renderer(); }
inline const ItemAllocation& Drawable::allocation() const  { return _gpuAllocation; }
inline uint32_t Drawable::shaderDataID() const  { return _shaderDataID; }
inline StateSet* Drawable::stateSet() const  { return _stateSet; }
inline void Drawable::upload(const DrawableGpuData& drawableData)  { renderer()->uploadDrawable(*this,drawableData); }
inline StagingBuffer Drawable::createStagingBuffer()  { return renderer()->createDrawableStagingBuffer(*this); }

}
