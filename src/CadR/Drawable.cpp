#include <CadR/Drawable.h>
#include <CadR/DataAllocation.h>
#include <CadR/DataStorage.h>
#include <CadR/Geometry.h>
#include <CadR/PrimitiveSet.h>

using namespace std;
using namespace CadR;

static_assert(sizeof(DrawableGpuData)==24,
              "DrawableGpuData size is expected to be 24 bytes.");


Drawable::~Drawable() noexcept
{
	if(_indexIntoStateSet!=~0u)
		_stateSetDrawableContainer->removeDrawableUnsafe(*this);

	if(_shaderData)
		_shaderData->free();
}


void Drawable::destroy() noexcept
{
	if(_indexIntoStateSet!=~0u) {
		_stateSetDrawableContainer->removeDrawableUnsafe(*this);
		_drawableListHook.unlink();
		_indexIntoStateSet=~0u;
	}
	if(_shaderData) {
		_shaderData->free();
		_shaderData = nullptr;
	}
}


Drawable::Drawable(Geometry& geometry, uint32_t primitiveSetIndex,
                   size_t shaderDataSize, uint32_t numInstances, StateSet& stateSet)
	: Drawable()  // this ensures that the destructor is called if this constructor throws
{
	geometry._drawableList.push_back(*this);  // initializes _drawableListHook

	// alloc _shaderData
	if(shaderDataSize != 0)
		_shaderData = renderer()->dataStorage().alloc(shaderDataSize, moveCallback, this);

	// append into StateSet
	// (it initializes _stateSetDrawableContainer and _indexIntoStateSet)
	GeometryMemory* m = geometry.geometryMemory();
	stateSet.appendDrawableUnsafe(
		*this,  // drawable
		DrawableGpuData(  // drawableGpuData
			m->bufferDeviceAddress() + m->primitiveSetOffset() +
				(geometry.primitiveSetAllocation().startIndex + primitiveSetIndex) *
					(uint32_t(sizeof(CadR::PrimitiveSetGpuData))),  // primitiveSetAddr
			_shaderData->deviceAddress(),  // shaderDataAddr
			numInstances  // numInstances
		),
		m->id()  // geometryMemoryId
	);
}


Drawable::Drawable(Drawable&& other) noexcept
{
	_stateSetDrawableContainer = other._stateSetDrawableContainer;
	_indexIntoStateSet = other._indexIntoStateSet;
	_stateSetDrawableContainer->drawablePtrList[_indexIntoStateSet] = this;
	other._indexIntoStateSet = ~0;
	_shaderData = other._shaderData;
	other._shaderData = nullptr;
	_drawableListHook = std::move(other._drawableListHook);
}


Drawable& Drawable::operator=(Drawable&& rhs) noexcept
{
	if(_indexIntoStateSet != ~0u)
		_stateSetDrawableContainer->removeDrawableUnsafe(*this);
	_stateSetDrawableContainer = rhs._stateSetDrawableContainer;
	_indexIntoStateSet = rhs._indexIntoStateSet;
	_stateSetDrawableContainer->drawablePtrList[_indexIntoStateSet] = this;
	rhs._indexIntoStateSet = ~0;
	if(_shaderData)
		_shaderData->free();
	_shaderData = rhs._shaderData;
	rhs._shaderData = nullptr;
	_drawableListHook = std::move(rhs._drawableListHook);
	return *this;
}


void Drawable::moveCallback(DataAllocation* oldAlloc, DataAllocation* newAlloc, void* userData)
{
	static_cast<Drawable*>(userData)->_shaderData = newAlloc;
}


void Drawable::create(Geometry& geometry, uint32_t primitiveSetIndex,
                      size_t shaderDataSize, uint32_t numInstances, StateSet& stateSet)
{
	// bind with new geometry
	geometry._drawableList.push_back(*this);

	// alloc new _shaderData
	if(_shaderData) {
		_shaderData->free();
		_shaderData = nullptr;
	}
	if(shaderDataSize != 0)
		_shaderData = stateSet.renderer()->dataStorage().alloc(shaderDataSize, moveCallback, this);

	// handle previous bindings 
	GeometryMemory* m = geometry.geometryMemory();
	if(_indexIntoStateSet != ~0u)
		if(_stateSetDrawableContainer->geometryMemory == m) {

			// only update DrawableGpuData
			_stateSetDrawableContainer->drawableDataList[_indexIntoStateSet] =
				DrawableGpuData(
					m->bufferDeviceAddress() + m->primitiveSetOffset() +
						((geometry.primitiveSetAllocation().startIndex + primitiveSetIndex) *
							(uint32_t(sizeof(CadR::PrimitiveSetGpuData)))),  // primitiveSetAddr
					_shaderData->deviceAddress(),  // shaderDataAddr
					numInstances  // numInstances
				);
			return;

		}
		else
			_stateSetDrawableContainer->removeDrawableUnsafe(*this);

	// append into StateSet
	// (it initializes _stateSetDrawableContainer and _indexIntoStateSet)
	stateSet.appendDrawableUnsafe(
		*this,  // drawable
		DrawableGpuData(  // drawableGpuData
			m->bufferDeviceAddress() + m->primitiveSetOffset() +
				((geometry.primitiveSetAllocation().startIndex + primitiveSetIndex) *
					(uint32_t(sizeof(CadR::PrimitiveSetGpuData)))),  // primitiveSetAddr
			_shaderData->deviceAddress(),  // shaderDataAddr
			numInstances  // numInstances
		),
		m->id()  // geometryMemoryId
	);
}


#if 0 // not updated to new data allocation structures yet
void Drawable::create(Geometry& geometry, uint32_t primitiveSetIndex)
{
	if(_indexIntoStateSet == ~0u)
		throw runtime_error("Drawable::create(geometry, primitiveSetIndex) can be used only "
		                    "for replacing geometry and primitiveSetIndex in the valid Drawable. "
		                    "Use Drawable::create(geometry, primitiveSetIndex, shaderDataSize, numInstances, stateSet) instead.");

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
				_shaderData->deviceAddress()  // shaderDataAddr
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
			_shaderData->deviceAddress()  // shaderDataAddr
		),
		m->id()  // geometryMemoryId
	);
}
#endif


void Drawable::allocShaderData(size_t size, uint32_t numInstances)
{
	// alloc new _shaderData
	if(_shaderData) {
		_shaderData->free();
		_shaderData = nullptr;
	}
	if(size != 0)
		_shaderData = renderer()->dataStorage().alloc(size, moveCallback, this);

	// update DrawableGpuData
	if(_indexIntoStateSet == ~0u)
		throw runtime_error("Drawable::allocShaderData(size, numInstances) can be used only "
		                    "for updating the valid Drawable. "
		                    "Use Drawable::create(geometry, primitiveSetIndex, shaderDataSize, numInstances, stateSet) first.");
	DrawableGpuData& d = _stateSetDrawableContainer->drawableDataList[_indexIntoStateSet];
	d.shaderDataAddr = _shaderData ? _shaderData->deviceAddress() : 0;
	d.numInstances = numInstances;
}


void Drawable::allocShaderData(size_t size)
{
	// alloc new _shaderData
	if(_shaderData) {
		_shaderData->free();
		_shaderData = nullptr;
	}
	if(size != 0)
		_shaderData = renderer()->dataStorage().alloc(size, moveCallback, this);

	// update DrawableGpuData
	if(_indexIntoStateSet != ~0u)
		_stateSetDrawableContainer->drawableDataList[_indexIntoStateSet].shaderDataAddr =
			_shaderData ? _shaderData->deviceAddress() : 0;
}


void Drawable::freeShaderData()
{
	// release _shaderData
	if(_shaderData) {
		_shaderData->free();
		_shaderData = nullptr;
	}

	// update DrawableGpuData
	if(_indexIntoStateSet != ~0u)
		_stateSetDrawableContainer->drawableDataList[_indexIntoStateSet].shaderDataAddr = 0;
}


StagingData Drawable::createStagingData()
{
	if(_shaderData == nullptr)
		throw runtime_error("Drawable::createStagingData(): No shader data allocated.");

	return _shaderData->createStagingData();
}
