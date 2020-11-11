#include <CadR/Drawable.h>
#include <CadR/Geometry.h>
#include <CadR/PrimitiveSet.h>

using namespace CadR;

static_assert(sizeof(DrawableGpuData)==8,
              "DrawableGpuData size is expected to be 8 bytes.");


Drawable::Drawable(Geometry& geometry,uint32_t primitiveSetIndex,
                   uint32_t shaderDataID,StateSet& stateSet)
	: _shaderDataID(shaderDataID)
{
	geometry._drawableList.push_back(*this);
	stateSet.appendDrawableUnsafe(
		*this,
		DrawableGpuData(
			(geometry.primitiveSetAllocation().startIndex+primitiveSetIndex)*(uint32_t(sizeof(CadR::PrimitiveSetGpuData))/4),  // primitiveSetOffset4
			shaderDataID/4  // shaderDataOffset4
		)
	);
}


Drawable::Drawable(Drawable&& other)
{
	_stateSet=other._stateSet;
	_indexIntoStateSet=other._indexIntoStateSet;
	_stateSet->_drawableList[_indexIntoStateSet]=this;
	other._indexIntoStateSet=~0;
	_shaderDataID=other._shaderDataID;
	_drawableListHook=std::move(other._drawableListHook);
}


Drawable& Drawable::operator=(Drawable&& rhs)
{
	if(_indexIntoStateSet!=~0)
		_stateSet->removeDrawableUnsafe(*this);
	_stateSet=rhs._stateSet;
	_indexIntoStateSet=rhs._indexIntoStateSet;
	_stateSet->_drawableList[_indexIntoStateSet]=this;
	rhs._indexIntoStateSet=~0;
	_shaderDataID=rhs._shaderDataID;
	_drawableListHook=std::move(rhs._drawableListHook);
	return *this;
}


void Drawable::create(Geometry& geometry,uint32_t primitiveSetIndex,
                      uint32_t shaderDataID,StateSet& stateSet)
{
	geometry._drawableList.push_back(*this);
	_shaderDataID=shaderDataID;
	if(_indexIntoStateSet!=~0)
		if(_stateSet==&stateSet) {
			_stateSet->_drawableDataList[_indexIntoStateSet]=
				DrawableGpuData(
					(geometry.primitiveSetAllocation().startIndex+primitiveSetIndex)*(uint32_t(sizeof(CadR::PrimitiveSetGpuData))/4),  // primitiveSetOffset4
					shaderDataID/4  // shaderDataOffset4
				);
			return;
		}
		else
			_stateSet->removeDrawableUnsafe(*this);

	_stateSet=&stateSet;
	_stateSet->appendDrawableUnsafe(
		*this,
		DrawableGpuData(
			(geometry.primitiveSetAllocation().startIndex+primitiveSetIndex)*(uint32_t(sizeof(CadR::PrimitiveSetGpuData))/4),  // primitiveSetOffset4
			shaderDataID/4  // shaderDataOffset4
		)
	);
}


void Drawable::create(Geometry& geometry,uint32_t primitiveSetIndex)
{
	geometry._drawableList.push_back(*this);
	if(_indexIntoStateSet!=~0) {
		_stateSet->_drawableDataList[_indexIntoStateSet]=
			DrawableGpuData(
				(geometry.primitiveSetAllocation().startIndex+primitiveSetIndex)*(uint32_t(sizeof(CadR::PrimitiveSetGpuData))/4),  // primitiveSetOffset4
				_shaderDataID/4  // shaderDataOffset4
			);
	}
	else {
		_stateSet->appendDrawableUnsafe(
			*this,
			DrawableGpuData(
				(geometry.primitiveSetAllocation().startIndex+primitiveSetIndex)*(uint32_t(sizeof(CadR::PrimitiveSetGpuData))/4),  // primitiveSetOffset4
				_shaderDataID/4  // shaderDataOffset4
			)
		);
	}
}
