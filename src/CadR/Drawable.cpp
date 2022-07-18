#include <CadR/Drawable.h>
#include <CadR/Geometry.h>
#include <CadR/PrimitiveSet.h>

using namespace std;
using namespace CadR;

static_assert(sizeof(DrawableGpuData)==16,
              "DrawableGpuData size is expected to be 16 bytes.");


Drawable::Drawable(Geometry& geometry, uint32_t primitiveSetIndex,
                   uint32_t shaderDataID, StateSet& stateSet)
	: _shaderDataID(shaderDataID)
{
	geometry._drawableList.push_back(*this);  // initializes _drawableListHook

	// append into StateSet
	// (it initializes _stateSetDrawableContainer and _indexIntoStateSet)
	GeometryMemory* m = geometry.geometryMemory();
	stateSet.appendDrawableUnsafe(
		*this,  // drawable
		DrawableGpuData(  // drawableGpuData
			m->bufferDeviceAddress() + m->primitiveSetOffset() +
				(geometry.primitiveSetAllocation().startIndex + primitiveSetIndex) *
					(uint32_t(sizeof(CadR::PrimitiveSetGpuData))),  // primitiveSetAddr
			shaderDataID/4  // shaderDataOffset4
		),
		m->id()  // geometryMemoryId
	);
}


Drawable::Drawable(Drawable&& other)
{
	_stateSetDrawableContainer = other._stateSetDrawableContainer;
	_indexIntoStateSet = other._indexIntoStateSet;
	_stateSetDrawableContainer->drawablePtrList[_indexIntoStateSet] = this;
	other._indexIntoStateSet = ~0;
	_shaderDataID = other._shaderDataID;
	_drawableListHook = std::move(other._drawableListHook);
}


Drawable& Drawable::operator=(Drawable&& rhs)
{
	if(_indexIntoStateSet != ~0u)
		_stateSetDrawableContainer->removeDrawableUnsafe(*this);
	_stateSetDrawableContainer = rhs._stateSetDrawableContainer;
	_indexIntoStateSet = rhs._indexIntoStateSet;
	_stateSetDrawableContainer->drawablePtrList[_indexIntoStateSet] = this;
	rhs._indexIntoStateSet = ~0;
	_shaderDataID = rhs._shaderDataID;
	_drawableListHook = std::move(rhs._drawableListHook);
	return *this;
}


void Drawable::create(Geometry& geometry, uint32_t primitiveSetIndex,
                      uint32_t shaderDataID, StateSet& stateSet)
{
	// bind with new geometry and new shaderDataID
	geometry._drawableList.push_back(*this);
	_shaderDataID = shaderDataID;

	// handle previous bindings 
	GeometryMemory* m = geometry.geometryMemory();
	if(_indexIntoStateSet != ~0u) {
		if(_stateSetDrawableContainer->geometryMemory == m) {

			// only update DrawableGpuData
			_stateSetDrawableContainer->drawableDataList[_indexIntoStateSet]=
				DrawableGpuData(
					m->bufferDeviceAddress() + m->primitiveSetOffset() +
						(geometry.primitiveSetAllocation().startIndex + primitiveSetIndex) *
							(uint32_t(sizeof(CadR::PrimitiveSetGpuData))),  // primitiveSetAddr
					shaderDataID/4  // shaderDataOffset4
				);
			return;

		}
		else
			_stateSetDrawableContainer->removeDrawableUnsafe(*this);
	}

	// append into StateSet
	// (it initializes _stateSetDrawableContainer and _indexIntoStateSet)
	stateSet.appendDrawableUnsafe(
		*this,  // drawable
		DrawableGpuData(  // drawableGpuData
			m->bufferDeviceAddress() + m->primitiveSetOffset() +
				(geometry.primitiveSetAllocation().startIndex + primitiveSetIndex) *
					(uint32_t(sizeof(CadR::PrimitiveSetGpuData))),  // primitiveSetAddr
			shaderDataID/4  // shaderDataOffset4
		),
		m->id()  // geometryMemoryId
	);
}


void Drawable::create(Geometry& geometry, uint32_t primitiveSetIndex)
{
	if(_indexIntoStateSet == ~0u)
		throw runtime_error("Drawable::create(geometry, primitiveSetIndex) can be used only "
		                    "for replacing geometry and primitiveSetIndex in the valid Drawable. "
		                    "Use Drawable::create(geometry, primitiveSetIndex, shaderDataID, stateSet) instead.");

	// bind with new geometry
	geometry._drawableList.push_back(*this);

	// handle previous bindings
	GeometryMemory* m = geometry.geometryMemory();
	if(_stateSetDrawableContainer->geometryMemory == m) {

		// update DrawableGpuData
		_stateSetDrawableContainer->drawableDataList[_indexIntoStateSet]=
			DrawableGpuData(
				m->bufferDeviceAddress() + m->primitiveSetOffset() +
					(geometry.primitiveSetAllocation().startIndex + primitiveSetIndex) *
						(uint32_t(sizeof(CadR::PrimitiveSetGpuData))),  // primitiveSetAddr
				_shaderDataID/4  // shaderDataOffset4
			);
		return;

	} else
		_stateSetDrawableContainer->removeDrawableUnsafe(*this);

	// append into StateSet
	// (it initializes _stateSetDrawableContainer and _indexIntoStateSet)
	_stateSetDrawableContainer->stateSet->appendDrawableUnsafe(
		*this,  // drawable
		DrawableGpuData(  // drawableGpuData
			m->bufferDeviceAddress() + m->primitiveSetOffset() +
				(geometry.primitiveSetAllocation().startIndex + primitiveSetIndex) *
					(uint32_t(sizeof(CadR::PrimitiveSetGpuData))),  // primitiveSetAddr
			_shaderDataID/4  // shaderDataOffset4
		),
		m->id()  // geometryMemoryId
	);
}
