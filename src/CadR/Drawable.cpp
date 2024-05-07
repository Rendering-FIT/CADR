#include <CadR/Drawable.h>
#include <CadR/DataAllocation.h>
#include <CadR/DataStorage.h>
#include <CadR/Geometry.h>
#include <CadR/PrimitiveSet.h>

using namespace std;
using namespace CadR;

static_assert(sizeof(DrawableGpuData)==40,
              "DrawableGpuData size is expected to be 40 bytes. Otherwise updates to "
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
		_shaderData.free();
		_shaderData.destroyHandle();
	}
}


Drawable::Drawable(Geometry& geometry, uint32_t primitiveSetOffset,
                   uint32_t numInstances, StateSet& stateSet)
	: Drawable(geometry.renderer())  // completion of this constructor ensures the destructor is called even if this constructor throws
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
			geometry.primitiveSetDataAllocation().handle(),  // primitiveSetHandle
			0,  // shaderDataHandle
			primitiveSetOffset,  // primitiveSetOffset
			numInstances  // numInstances
		)
	);
}


Drawable::Drawable(Geometry& geometry, uint32_t primitiveSetOffset,
                   StagingData& shaderStagingData, size_t shaderDataSize,
                   uint32_t numInstances, StateSet& stateSet)
	: Drawable(geometry.renderer())  // completion of this constructor ensures the destructor is called even if this constructor throws
{
	// shader data
	_shaderData.createHandle(geometry.dataStorage());
	shaderStagingData = _shaderData.alloc(shaderDataSize);

	// register in Geometry's drawable list
	geometry._drawableList.push_back(*this);  // initializes _drawableListHook

	// append into StateSet
	// (it initializes _stateSet and _indexIntoStateSet)
	stateSet.appendDrawableInternal(
		*this,  // drawable
		DrawableGpuData(  // drawableGpuData
			geometry.vertexDataAllocation().handle(),  // vertexDataHandle
			geometry.indexDataAllocation().handle(),  // indexDataHandle
			geometry.primitiveSetDataAllocation().handle(),  // primitiveSetHandle
			_shaderData.handle(),  // shaderDataHandle
			primitiveSetOffset,  // primitiveSetOffset
			numInstances  // numInstances
		)
	);
}


Drawable::Drawable(Drawable&& other) noexcept
	: _stateSet(other._stateSet)
	, _shaderData(std::move(other._shaderData))
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
	_indexIntoStateSet = rhs._indexIntoStateSet;
	_stateSet->_drawablePtrList[_indexIntoStateSet] = this;
	rhs._indexIntoStateSet = ~0;
	_shaderData = std::move(rhs._shaderData);
	_drawableListHook = std::move(rhs._drawableListHook);
	return *this;
}


StagingData Drawable::create(Geometry& geometry, uint32_t primitiveSetOffset,
                             size_t shaderDataSize, uint32_t numInstances, StateSet& stateSet)
{
	// bind with new geometry
	geometry._drawableList.push_back(*this);

	// alloc new _shaderData
	_shaderData.createHandle(geometry.dataStorage());
	StagingData sd = _shaderData.alloc(shaderDataSize);

	// handle previous bindings
	if(_indexIntoStateSet != ~0u) {
		if(_stateSet == &stateSet) {

			// only update DrawableGpuData
			_stateSet->_drawableDataList[_indexIntoStateSet] =
				DrawableGpuData(  // drawableGpuData
					geometry.vertexDataAllocation().handle(),  // vertexDataHandle
					geometry.indexDataAllocation().handle(),  // indexDataHandle
					geometry.primitiveSetDataAllocation().handle(),  // primitiveSetHandle
					_shaderData.handle(),  // shaderDataHandle
					primitiveSetOffset,  // primitiveSetOffset
					numInstances  // numInstances
				);
			return sd;

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
			geometry.primitiveSetDataAllocation().handle(),  // primitiveSetHandle
			_shaderData.handle(),  // shaderDataHandle
			primitiveSetOffset,  // primitiveSetOffset
			numInstances  // numInstances
		)
	);

	return sd;
}


void Drawable::create(Geometry& geometry, uint32_t primitiveSetOffset,
                      uint32_t numInstances, StateSet& stateSet)
{
	// bind with new geometry
	geometry._drawableList.push_back(*this);

	// free _shaderData
	_shaderData.free();
	_shaderData.destroyHandle();

	// handle previous bindings
	if(_indexIntoStateSet != ~0u) {
		if(_stateSet == &stateSet) {

			// only update DrawableGpuData
			_stateSet->_drawableDataList[_indexIntoStateSet] =
				DrawableGpuData(  // drawableGpuData
					geometry.vertexDataAllocation().handle(),  // vertexDataHandle
					geometry.indexDataAllocation().handle(),  // indexDataHandle
					geometry.primitiveSetDataAllocation().handle(),  // primitiveSetHandle
					0,  // shaderDataHandle
					primitiveSetOffset,  // primitiveSetOffset
					numInstances  // numInstances
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
			geometry.primitiveSetDataAllocation().handle(),  // primitiveSetHandle
			0,  // shaderDataHandle
			primitiveSetOffset,  // primitiveSetOffset
			numInstances  // numInstances
		)
	);
}


StagingData Drawable::reallocShaderData(size_t size, uint32_t numInstances)
{
	if(_indexIntoStateSet == ~0u || _shaderData.handle() == 0)
		throw runtime_error("Drawable::reallocShaderData(size, numInstances) can be used only "
		                    "for updating the valid Drawable that was created with shader data. "
		                    "Use Drawable::create(geometry, primitiveSetOffset, shaderDataSize, numInstances, stateSet) first.");

	StagingData sd = _shaderData.alloc(size);
	DrawableGpuData& d = _stateSet->_drawableDataList[_indexIntoStateSet];
	d.numInstances = numInstances;
	return sd;
}


StagingData Drawable::reallocShaderData(size_t size)
{
	if(_indexIntoStateSet == ~0u || _shaderData.handle() == 0)
		throw runtime_error("Drawable::reallocShaderData(size, numInstances) can be used only "
		                    "for updating the valid Drawable that was created with shader data. "
		                    "Use Drawable::create(geometry, primitiveSetOffset, shaderDataSize, numInstances, stateSet) first.");

	StagingData sd = _shaderData.alloc(size);
	return sd;
}


void Drawable::freeShaderData()
{
	_shaderData.free();
}
