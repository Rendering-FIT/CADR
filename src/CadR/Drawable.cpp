#include <CadR/Drawable.h>
#include <CadR/Geometry.h>
#include <CadR/PrimitiveSet.h>

using namespace CadR;

static_assert(sizeof(DrawableGpuData)==16,
              "DrawableGpuData size is expected to be 16 bytes.");


Drawable::Drawable(Geometry* geometry,uint32_t primitiveSetIndex,
                   uint32_t shaderDataID,StateSet* stateSet,uint32_t userData)
	: _shaderDataID(shaderDataID), _stateSet(stateSet)
{
	geometry->_drawableList.push_back(*this);
	_gpuAllocation.alloc(stateSet->renderer()->drawableAllocationManager());
	stateSet->incrementNumDrawables();
	upload(
		DrawableGpuData{
			(geometry->primitiveSetAllocation().startIndex+primitiveSetIndex)*(uint32_t(sizeof(CadR::PrimitiveSetGpuData))/4),  // primitiveSetOffset4
			shaderDataID/4,  // shaderDataOffset4
			stateSet->id(),  // stateSetOffset4
			userData  // userData
		});
}


Drawable::Drawable(Drawable&& other)
{
	_gpuAllocation.moveFrom(other._gpuAllocation,other._stateSet->renderer()->drawableAllocationManager());
	_shaderDataID=other._shaderDataID;
	_stateSet=other._stateSet;
	_drawableListHook=std::move(other._drawableListHook);
}


Drawable& Drawable::operator=(Drawable&& rhs)
{
	// move _gpuAllocation, increment and decrement StateSet::numDrawables
	if(_gpuAllocation.isValid()) {
		_stateSet->decrementNumDrawables();
		if(_stateSet!=rhs._stateSet)
			_gpuAllocation.free(_stateSet->renderer()->drawableAllocationManager());
	}
	_gpuAllocation.moveFrom(rhs._gpuAllocation,rhs._stateSet->renderer()->drawableAllocationManager());  // this is safe even on already allocated _gpuAllocation
	if(_gpuAllocation.isValid())
		rhs._stateSet->incrementNumDrawables();

	// move remaining variables
	_shaderDataID=rhs._shaderDataID;
	_stateSet=rhs._stateSet;
	_drawableListHook=std::move(rhs._drawableListHook);
	return *this;
}


void Drawable::create(Geometry* geometry,uint32_t primitiveSetIndex,
                      uint32_t shaderDataID,StateSet* stateSet,uint32_t userData)
{
	if(_gpuAllocation.isValid()) {
		_stateSet->decrementNumDrawables();
		if(_stateSet!=stateSet)
			_gpuAllocation.free(_stateSet->renderer()->drawableAllocationManager());
	}
	_gpuAllocation.alloc(stateSet->renderer()->drawableAllocationManager());  // this is safe even on already allocated _gpuAllocation
	stateSet->incrementNumDrawables();
	_shaderDataID=shaderDataID;
	_stateSet=stateSet;
	geometry->_drawableList.push_back(*this);
	upload(
		DrawableGpuData{
			(geometry->primitiveSetAllocation().startIndex+primitiveSetIndex)*(uint32_t(sizeof(CadR::PrimitiveSetGpuData))/4),  // primitiveSetOffset4
			shaderDataID/4,  // shaderDataOffset4
			stateSet->id(),  // stateSetOffset4
			userData  // userData
		});
}


void Drawable::create(Geometry* geometry,uint32_t primitiveSetIndex,uint32_t userData)
{
	if(!_gpuAllocation.isValid())
		_stateSet->incrementNumDrawables();
	_gpuAllocation.alloc(renderer()->drawableAllocationManager());  // this is safe even on already allocated _gpuAllocation
	geometry->_drawableList.push_back(*this);
	upload(
		DrawableGpuData{
			(geometry->primitiveSetAllocation().startIndex+primitiveSetIndex)*(uint32_t(sizeof(CadR::PrimitiveSetGpuData))/4),  // primitiveSetOffset4
			_shaderDataID/4,  // shaderDataOffset4
			_stateSet->id(),  // stateSetOffset4
			userData  // userData
		});
}
