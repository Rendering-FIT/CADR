#include <CadR/Mesh.h>

using namespace CadR;


Mesh::Mesh(Mesh&& m)
	: _attribStorage(m._attribStorage)
	, _attribDataID(m._attribDataID)
	, _indexDataID(m._indexDataID)
	, _primitiveSetDataID(m._primitiveSetDataID)
	, _drawableList(std::move(m._drawableList))
{
	Renderer* r=renderer();
	m._attribStorage=r->emptyStorage();
	m._attribDataID=0;
	m._indexDataID=0;
	m._primitiveSetDataID=0;
	_attribStorage->attribAllocation(_attribDataID).owner=this;
	r->indexAllocation(_indexDataID).owner=this;
	r->primitiveSetAllocation(_primitiveSetDataID).owner=this;
}


Mesh& Mesh::operator=(Mesh&& m)
{
	Renderer* r=renderer();
	_attribStorage=m._attribStorage;
	m._attribStorage=r->emptyStorage();
	_attribDataID=m._attribDataID;
	_indexDataID=m._indexDataID;
	_primitiveSetDataID=m._primitiveSetDataID;
	_drawableList=std::move(m._drawableList);
	m._attribDataID=0;
	m._indexDataID=0;
	m._primitiveSetDataID=0;
	_attribStorage->attribAllocation(_attribDataID).owner=this;
	r->indexAllocation(_indexDataID).owner=this;
	r->primitiveSetAllocation(_primitiveSetDataID).owner=this;
	return *this;
}
