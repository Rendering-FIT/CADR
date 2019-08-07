#include <CadR/Renderer.h>
#include <CadR/AttribStorage.h>
#include <CadR/AllocationManagers.h>

using namespace std;
using namespace CadR;

unique_ptr<Renderer> Renderer::_instance;



Renderer::Renderer()
	: _attribStorages{{AttribConfig(),{AttribStorage(this,AttribConfig())}}}
	, _emptyStorage(&_attribStorages.begin()->second.front())
	, _indexAllocationManager(0,0)  // zero capacity, zero-sized object on index 0
{
}


Renderer::~Renderer()
{
	if(_device)
		_device.destroy();

	assert(_emptyStorage->allocationManager().numIDs()==1 && "Renderer::_emptyStorage is not empty. It is a programmer error to allocate anything there. You probably called Mesh::allocAttribs() without specifying AttribConfig.");
}


AttribStorage* Renderer::getOrCreateAttribStorage(const AttribConfig& ac)
{
	std::list<AttribStorage>& attribStorageList=_attribStorages[ac];
	if(attribStorageList.empty())
		attribStorageList.emplace_front(AttribStorage(this,ac));
	return &attribStorageList.front();
}
