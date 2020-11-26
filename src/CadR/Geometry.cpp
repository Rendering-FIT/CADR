#include <CadR/Geometry.h>

using namespace CadR;


Geometry::Geometry(Geometry&& g)
	: _vertexStorage(g._vertexStorage)
	, _vertexDataID(g._vertexDataID)
	, _indexDataID(g._indexDataID)
	, _primitiveSetDataID(g._primitiveSetDataID)
	, _drawableList(std::move(g._drawableList))
{
	Renderer* r=renderer();
	g._vertexStorage=r->emptyStorage();
	g._vertexDataID=0;
	g._indexDataID=0;
	g._primitiveSetDataID=0;
	vertexAllocation().owner=this;
	indexAllocation().owner=this;
	primitiveSetAllocation().owner=this;
}


Geometry& Geometry::operator=(Geometry&& g)
{
	Renderer* r=renderer();
	_vertexStorage=g._vertexStorage;
	g._vertexStorage=r->emptyStorage();
	_vertexDataID=g._vertexDataID;
	_indexDataID=g._indexDataID;
	_primitiveSetDataID=g._primitiveSetDataID;
	_drawableList=std::move(g._drawableList);
	g._vertexDataID=0;
	g._indexDataID=0;
	g._primitiveSetDataID=0;
	vertexAllocation().owner=this;
	indexAllocation().owner=this;
	primitiveSetAllocation().owner=this;
	return *this;
}
