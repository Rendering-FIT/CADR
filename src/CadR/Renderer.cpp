#include <CadR/Renderer.h>
#include <CadR/AttribStorage.h>
#include <CadR/AllocationManagers.h>

using namespace std;
using namespace CadR;

Renderer* Renderer::_instance = nullptr;



Renderer::Renderer(VulkanDevice* device)
	: _device(device)
	, _indexAllocationManager(0,0)  // zero capacity, zero-sized object on index 0
{
	_attribStorages[AttribConfig()].emplace_back(this,AttribConfig()); // create empty AttribStorage for empty AttribConfig
	_emptyStorage=&_attribStorages.begin()->second.front();
}


Renderer::~Renderer()
{
	assert(_emptyStorage->allocationManager().numIDs()==1 && "Renderer::_emptyStorage is not empty. It is a programmer error to allocate anything there. You probably called Mesh::allocAttribs() without specifying AttribConfig.");
}


AttribStorage* Renderer::getOrCreateAttribStorage(const AttribConfig& ac)
{
	std::list<AttribStorage>& attribStorageList=_attribStorages[ac];
	if(attribStorageList.empty())
		attribStorageList.emplace_front(AttribStorage(this,ac));
	return &attribStorageList.front();
}
