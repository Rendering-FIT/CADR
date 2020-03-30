#include <CadR/Geometry.h>

using namespace CadR;


Geometry::Geometry(Geometry&& g)
	: _attribStorage(g._attribStorage)
	, _attribDataID(g._attribDataID)
	, _indexDataID(g._indexDataID)
	, _primitiveSetDataID(g._primitiveSetDataID)
	, _drawableList(std::move(g._drawableList))
{
	Renderer* r=renderer();
	g._attribStorage=r->emptyStorage();
	g._attribDataID=0;
	g._indexDataID=0;
	g._primitiveSetDataID=0;
	_attribStorage->attribAllocation(_attribDataID).owner=this;
	r->indexAllocation(_indexDataID).owner=this;
	r->primitiveSetAllocation(_primitiveSetDataID).owner=this;
}


Geometry& Geometry::operator=(Geometry&& g)
{
	Renderer* r=renderer();
	_attribStorage=g._attribStorage;
	g._attribStorage=r->emptyStorage();
	_attribDataID=g._attribDataID;
	_indexDataID=g._indexDataID;
	_primitiveSetDataID=g._primitiveSetDataID;
	_drawableList=std::move(g._drawableList);
	g._attribDataID=0;
	g._indexDataID=0;
	g._primitiveSetDataID=0;
	_attribStorage->attribAllocation(_attribDataID).owner=this;
	r->indexAllocation(_indexDataID).owner=this;
	r->primitiveSetAllocation(_primitiveSetDataID).owner=this;
	return *this;
}
