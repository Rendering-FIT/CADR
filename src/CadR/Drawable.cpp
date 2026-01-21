#include <CadR/Drawable.h>
#include <CadR/DataAllocation.h>
#include <CadR/Geometry.h>
#include <CadR/MatrixList.h>

using namespace std;
using namespace CadR;

static_assert(sizeof(DrawableGpuData)==48,
              "DrawableGpuData size is expected to be 48 bytes. Otherwise updates to "
              "Drawable class and to processDrawables.comp shader might be necessary.");


Drawable::~Drawable() noexcept
{
	if(_indexIntoStateSet != ~0u)
		_stateSet->removeDrawableInternal(*this);
}


void Drawable::destroy() noexcept
{
	if(_indexIntoStateSet != ~0u) {
		_stateSet->removeDrawableInternal(*this);
		_drawableListHook.unlink();
		_indexIntoStateSet = ~0u;
	}
}


Drawable::Drawable(Geometry& geometry, uint32_t primitiveSetOffset,
                   MatrixList& matrixList, StateSet& stateSet)
	: Drawable(&matrixList, nullptr)  // completion of this constructor ensures the destructor is called even if this constructor throws
{
	// register in Geometry's drawable list
	geometry._drawableList.push_back(*this);  // initializes _drawableListHook

	// append into StateSet
	// (it initializes _stateSet and _indexIntoStateSet)
	stateSet.appendDrawableInternal(
		*this,  // drawable
		DrawableGpuData(  // drawableGpuData
			geometry.vertexDataAllocation().handle(),  // vertexDataHandle
			geometry.indexDataAllocation().handle(),  // indexDataHandle
			matrixList.handle(),  // matrixListHandle
			0,  // drawableDataHandle
			geometry.primitiveSetDataAllocation().handle(),  // primitiveSetHandle
			primitiveSetOffset  // primitiveSetOffset
		)
	);
}


Drawable::Drawable(Geometry& geometry, uint32_t primitiveSetOffset, MatrixList& matrixList,
                   DataAllocation& drawableData, StateSet& stateSet)
	: Drawable(&matrixList, &drawableData)  // completion of this constructor ensures the destructor is called even if this constructor throws
{
	// register in Geometry's drawable list
	geometry._drawableList.push_back(*this);  // initializes _drawableListHook

	// append into StateSet
	// (it initializes _stateSet and _indexIntoStateSet)
	stateSet.appendDrawableInternal(
		*this,  // drawable
		DrawableGpuData(  // drawableGpuData
			geometry.vertexDataAllocation().handle(),  // vertexDataHandle
			geometry.indexDataAllocation().handle(),  // indexDataHandle
			matrixList.handle(),  // matrixListHandle
			drawableData.handle(),  // drawableDataHandle
			geometry.primitiveSetDataAllocation().handle(),  // primitiveSetHandle
			primitiveSetOffset  // primitiveSetOffset
		)
	);
}


Drawable::Drawable(Drawable&& other) noexcept
	: _stateSet(other._stateSet)
	, _matrixList(other._matrixList)
	, _drawableData(other._drawableData)
	, _indexIntoStateSet(other._indexIntoStateSet)
	, _drawableListHook(std::move(other._drawableListHook))
{
	_stateSet->_drawablePtrList[_indexIntoStateSet] = this;
	other._indexIntoStateSet = ~0;
}


Drawable& Drawable::operator=(Drawable&& rhs) noexcept
{
	if(_indexIntoStateSet != ~0u)
		_stateSet->removeDrawableInternal(*this);
	_stateSet = rhs._stateSet;
	_matrixList = rhs._matrixList;
	_drawableData = rhs._drawableData;
	_indexIntoStateSet = rhs._indexIntoStateSet;
	_stateSet->_drawablePtrList[_indexIntoStateSet] = this;
	rhs._indexIntoStateSet = ~0;
	_drawableListHook = std::move(rhs._drawableListHook);
	return *this;
}


void Drawable::create(Geometry& geometry, uint32_t primitiveSetOffset, MatrixList& matrixList,
                      DataAllocation* drawableData, StateSet& stateSet)
{
	// bind with new geometry
	geometry._drawableList.push_back(*this);

	// set members
	_matrixList = &matrixList;
	_drawableData = drawableData;

	// handle previous bindings
	if(_indexIntoStateSet != ~0u) {
		if(_stateSet == &stateSet) {

			// only update DrawableGpuData
			_stateSet->_drawableDataList[_indexIntoStateSet] =
				DrawableGpuData(  // drawableGpuData
					geometry.vertexDataAllocation().handle(),  // vertexDataHandle
					geometry.indexDataAllocation().handle(),  // indexDataHandle
					matrixList.handle(),  // matrixListHandle
					0,  // drawableDataHandle
					geometry.primitiveSetDataAllocation().handle(),  // primitiveSetHandle
					primitiveSetOffset  // primitiveSetOffset
				);
			return;

		}
		else
			_stateSet->removeDrawableInternal(*this);
	}

	// append into StateSet
	// (it initializes _stateSet and _indexIntoStateSet)
	stateSet.appendDrawableInternal(
		*this,  // drawable
		DrawableGpuData(  // drawableGpuData
			geometry.vertexDataAllocation().handle(),  // vertexDataHandle
			geometry.indexDataAllocation().handle(),  // indexDataHandle
			matrixList.handle(),  // matrixListHandle
			0,  // drawableDataHandle
			geometry.primitiveSetDataAllocation().handle(),  // primitiveSetHandle
			primitiveSetOffset  // primitiveSetOffset
		)
	);
}
