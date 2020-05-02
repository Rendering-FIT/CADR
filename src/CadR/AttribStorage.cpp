#include <CadR/AttribStorage.h>
#include <CadR/Renderer.h>
#include <CadR/StagingBuffer.h>
#include <CadR/VulkanDevice.h>
#include <iostream> // for cerr
#include <memory>

using namespace std;
using namespace CadR;



AttribStorage::AttribStorage(Renderer* renderer,const AttribSizeList& attribSizeList)
	: _allocationManager(1024,0)  // set capacity to 1024, zero-sized null object (on index 0)
	, _attribSizeList(attribSizeList)
	, _renderer(renderer)
{
#if 0
	//_renderer->onAttribStorageInit(this);
#endif

	// attribute buffers
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.flags=vk::BufferCreateFlags();
	bufferInfo.usage=vk::BufferUsageFlagBits::eVertexBuffer|vk::BufferUsageFlagBits::eTransferDst;
	bufferInfo.sharingMode=vk::SharingMode::eExclusive;
	bufferInfo.queueFamilyIndexCount=0;
	bufferInfo.pQueueFamilyIndices=nullptr;
	VulkanDevice* device=_renderer->device();
	_bufferList.reserve(_attribSizeList.size());
	for(uint8_t size : _attribSizeList) {

		// create buffer
		vk::Buffer b;
		if(size!=0) {
			bufferInfo.size=size*1024;
			b=device->createBuffer(bufferInfo);
		} else
			b=nullptr;
		_bufferList.push_back(b);

		// allocate memory
		vk::DeviceMemory m;
		if(size!=0) {
			m=_renderer->allocateMemory(b,vk::MemoryPropertyFlagBits::eDeviceLocal);
			_memoryList.push_back(m);
			device->bindBufferMemory(
					b,  // buffer
					m,  // memory
					0   // memoryOffset
				);
		}
		else
			_memoryList.push_back(nullptr);

	}

#if 0
	// create index buffer
	if(_attribConfig.indexed())
	{
		bufferInfo.usage=vk::BufferUsageFlagBits::eIndexBuffer;
		bufferInfo.size=4*1024;
		_indexBuffer=device->createBuffer(bufferInfo);
	}
	else
		_indexBuffer=nullptr;
#endif
}


AttribStorage::~AttribStorage()
{
	// invalidate all allocations and clean up
	cancelAllAllocations();
	//_renderer->onAttribStorageRelease(this);

	// destroy buffers
	VulkanDevice* device=_renderer->device();
	for(vk::Buffer b : _bufferList)
		if(b)
			device->destroy(b);

	// free memory
	for(vk::DeviceMemory m : _memoryList)
		if(m)
			device->freeMemory(m);
}


#if 0
/** Allocates the memory for vertices and indices
 *  inside AttribStorage.
 *
 *  Returns true on success. False on failure, usually caused by absence of
 *  large enough free memory block.
 *
 *  The method does not require active graphics context.
 *
 *  @param d Drawable that receives the allocation information.
 *           Any further references to the allocated memory are
 *           performed using the mesh passed in this parameter.
 *           The mesh must not have any allocated vertices or
 *           indices in the time of call of this method.
 *  @param numVertices number of vertices to be allocated
 *  @param numIndices number of indices to be allocated inside associated
 *                    Element Buffer Object
 */
bool AttribStorage::allocData(Drawable* d,unsigned numVertices,unsigned numIndices)
{
	// do we have enough space?
	if(!_vertexAllocationManager.canAllocate(numVertices) ||
	   !_indexAllocationManager.canAllocate(numIndices))
		return false;

	// allocate memory for vertices (inside AttribStorage's preallocated memory or buffers)
	unsigned verticesDataId=_vertexAllocationManager.alloc(numVertices,d);

	// allocate memory for indices (inside AttribStorage's preallocated memory or buffers)
	unsigned indicesDataId;
	if(numIndices==0)
		indicesDataId=0;
	else
		indicesDataId=_indexAllocationManager.alloc(numIndices,d);

	// update Drawable
	if(d->attribStorage()!=NULL)
	{
		cerr<<"Warning: calling AttribStorage::allocData() on Mesh\n"
		      "   that is not empty." << endl;
		d->freeData();
	}
	d->_attribStorage=this;
	d->_verticesDataId=verticesDataId;
	d->_indicesDataId=indicesDataId;

	return true;
}


/** Changes the number of allocated vertices and indices for the given Drawable.
 *
 *  numVertices and numIndices are the new number of elements in vertex and index buffers.
 *  If preserveContent parameter is true, the content of element and index data will be preserved.
 *  If new data are larger, the content over the size of previous data is undefined.
 *  If new data are smaller, only the data up to the new data size is preserved.
 *  If preserveContent is false, content of element and index data are undefined
 *  after reallocation.
 */
bool AttribStorage::reallocData(Drawable* /*d*/,unsigned /*numVertices*/,unsigned /*numIndices*/,
                                bool /*preserveContent*/)
{
	// Used strategy:
	// - if new arrays are smaller, we keep data in place and free the remaning space
	// - if new arrays are bigger and can be enlarged on the place, we do it
	// - otherwise we try to allocate new place for the data in the same AttribStorage
	// - if we do not succeed in the current AttribStorage, we move the data to
	//   some other AttribStorage
	// - if no AttribStorage accommodate us, we allocate new AttribStorage

	// FIXME: not implemented yet
	return false;
}


/** Releases the memory owned by Drawable.
 *
 *  This does not return the memory to the operating system.
 *  It just marks chunks of memory as available for next
 *  allocData() and reallocData() calls.
 *
 *  The method does not require active graphics context.
 */
void AttribStorage::freeData(Drawable* d)
{
	// check whether this attribStorage owns the given Mesh
	if(d->attribStorage()!=this)
	{
		cerr<<"Error: calling AttribStorage::freeData() on Drawable\n"
		      "   that is not managed by this AttribStorage."<<endl;
		return;
	}

	// make sure that there are no draw commands
	// (draw commands has to be freed first, otherwise we can not mark
	// Mesh as empty by setting attribStorage member to NULL)
	if(d->drawCommandDataId()!=0)
	{
		cerr<<"Error: calling AttribStorage::freeData() on Drawable\n"
		      "   that still has drawCommands allocated.\n"
		      "   Free drawCommands of associated Drawable first."<<endl;
		return;
	}

	// release vertices and indices allocations
	_vertexAllocationManager.free(d->verticesDataId());
	_indexAllocationManager.free(d->indicesDataId());

	// update Mesh
	d->_attribStorage=nullptr;
}


void AttribStorage::uploadVertices(Drawable* /*d*/,std::vector<Buffer>&& /*vertexData*/,size_t /*dstIndex*/)
{
#if 0
   auto& cfg=_attribConfig.configuration();
   unsigned c=unsigned(cfg.attribTypes.size());
   assert(c==attribListSize && "Number of attributes passed in parameters and stored inside "
                               "AttribStorage must match.");
   unsigned dstIndex=_vertexAllocationManager[mesh.verticesDataId()].startIndex+fromIndex;
   for(unsigned i=0,j=0; i<c; i++)
   {
      AttribType t=cfg.attribTypes[i];
      if(t==AttribType::Empty)
         continue;
      unsigned elementSize=t.elementSize();
      unsigned srcOffset=fromIndex*elementSize;
      unsigned dstOffset=dstIndex*elementSize;
      const void *data=attribList[i];
      _bufferList[j]->setData(((uint8_t*)data)+srcOffset,
                              numVertices*elementSize,dstOffset);
      j++;
   }
#endif
}
#endif


StagingBuffer AttribStorage::createStagingBuffer(Geometry& g,unsigned attribIndex)
{
	if(attribIndex>_bufferList.size())
		throw std::out_of_range("AttribStorage::createStagingBuffer() called with invalid attribIndex.");

	const ArrayAllocation<Geometry>& a=attribAllocation(g.attribDataID());
	const unsigned s=_attribSizeList[attribIndex];
	return StagingBuffer(
			_bufferList[attribIndex],  // dstBuffer
			a.startIndex*s,  // dstOffset
			a.numItems*s,  // size
			_renderer  // renderer
		);
}


StagingBuffer AttribStorage::createStagingBuffer(Geometry& g,unsigned attribIndex,size_t firstVertex,size_t numVertices)
{
	if(attribIndex>_bufferList.size())
		throw std::out_of_range("AttribStorage::createStagingBuffer() called with invalid attribIndex.");

	const ArrayAllocation<Geometry>& a=attribAllocation(g.attribDataID());
	if(firstVertex+numVertices>a.numItems)
		throw std::out_of_range("AttribStorage::createStagingBuffer() called with size and dstOffset that specify the range hitting outside of Geometry preallocated space.");

	const unsigned s=_attribSizeList[attribIndex];
	return StagingBuffer(
			_bufferList[attribIndex],  // dstBuffer
			(a.startIndex+firstVertex)*s,  // dstOffset
			numVertices*s,  // size
			_renderer  // renderer
		);
}


vector<StagingBuffer> AttribStorage::createStagingBuffers(Geometry& g)
{
	const ArrayAllocation<Geometry>& a=attribAllocation(g.attribDataID());
	vector<StagingBuffer> v;
	v.reserve(_bufferList.size());

	for(size_t i=0,e=_bufferList.size(); i<e; i++) {
		vk::Buffer b=_bufferList[i];
		const unsigned s=_attribSizeList[i];
		v.emplace_back(
				b,  // dstBuffer
				a.startIndex*s,  // dstOffset
				a.numItems*s,  // size
				_renderer  // renderer
			);
	}
	return v;
}


vector<StagingBuffer> AttribStorage::createStagingBuffers(Geometry& g,size_t firstVertex,size_t numVertices)
{
	const ArrayAllocation<Geometry>& a=attribAllocation(g.attribDataID());
	assert(a.numItems>=numVertices && "AttribStorage::createStagingBuffers(): Parameter numVertices is bigger than allocated space in Geometry.");
	vector<StagingBuffer> v;
	v.reserve(_bufferList.size());

	for(size_t i=0,e=_bufferList.size(); i<e; i++) {
		vk::Buffer b=_bufferList[i];
		const unsigned s=_attribSizeList[i];
		v.emplace_back(
				b,  // dstBuffer
				(a.startIndex+firstVertex)*s,  // dstOffset
				numVertices*s,  // size
				_renderer  // renderer
			);
	}
	return v;
}


void AttribStorage::uploadAttrib(Geometry& g,unsigned attribIndex,const std::vector<uint8_t>& attribData,size_t firstVertex)
{
	// attribIndex bound check
	if(attribIndex>_bufferList.size())
		throw std::out_of_range("AttribStorage::uploadAttrib() called with invalid attribIndex.");

	// create StagingBuffer and submit it
	size_t numVertices=attribData.size()/_attribSizeList[attribIndex];
	StagingBuffer sb(createStagingBuffer(g,attribIndex,firstVertex,numVertices));
	memcpy(sb.data(),attribData.data(),attribData.size());
	sb.submit();
}


void AttribStorage::uploadAttribs(Geometry& g,const vector<vector<uint8_t>>& vertexData,size_t firstVertex)
{
	// check parameters validity
	if(vertexData.size()!=_bufferList.size())
		throw std::out_of_range("AttribStorage::uploadAttribs() called with invalid vertexData.");
	if(vertexData.size()==0)
		return;
	size_t numVertices=vertexData[0].size()/_attribSizeList[0];
	for(size_t i=1,e=vertexData.size(); i<e; i++)
		if(vertexData[i].size()!=numVertices*_attribSizeList[i])
			throw std::out_of_range("AttribStorage::uploadAttribs() called with invalid vertexData.");

	// create StagingBuffers and submit them
	vector<StagingBuffer> sbList(createStagingBuffers(g,firstVertex,numVertices));
	for(size_t i=0,e=vertexData.size(); i<e; i++) {
		memcpy(sbList[i].data(),vertexData[i].data(),vertexData[i].size());
		sbList[i].submit();
	}
}


#if 0
void AttribStorage::uploadIndices(Drawable* /*d*/,std::vector<uint32_t>&& /*indexData*/,size_t /*dstIndex*/)
{
#if 0
   if(_eb==nullptr) {
      cout<<"Error in AttribStorage::uploadIndices(): ebo is null.\n"
            "   AttribStorage was probably created with AttribConfig\n"
            "   without ebo member set to true." << endl;
      return;
   }
   const unsigned elementSize=4;
   unsigned srcOffset=fromIndex*elementSize;
   unsigned dstOffset=(_indexAllocationManager[mesh.indicesDataId()].startIndex+fromIndex)*elementSize;
   _eb->setData((uint8_t*)indices+srcOffset,numIndices*elementSize,dstOffset);
#endif
}


#if 0
void AttribStorage::render(const std::vector<RenderingCommandData>& renderingDataList)
{
   // bind VAO
   bind();

   // perform indirect draw calls
   auto& gl=_va->getContext();
   if(attribConfig().configuration().ebo)
      for(auto it2=renderingDataList.begin(),e=renderingDataList.end(); it2!=e; it2++)
      {
         // call MultiDrawElementsIndirect
         GLintptr offset=it2->indirectBufferOffset4*4;
         gl.glMultiDrawElementsIndirect(it2->glMode,GL_UNSIGNED_INT,(const void*)offset,GLsizei(it2->drawCommandCount),0);
      }
   else
      for(auto it2=renderingDataList.begin(),e=renderingDataList.end(); it2!=e; it2++)
      {
         // call MultiDrawArraysIndirect
         GLintptr offset=it2->indirectBufferOffset4*4;
         gl.glMultiDrawArraysIndirect(it2->glMode,(const void*)offset,GLsizei(it2->drawCommandCount),0);
      }
}
#endif
#endif


void AttribStorage::cancelAllAllocations()
{
#if 0
	// break all Mesh references to this AttribStorage
	for(auto it=_allocationManager.begin(); it!=_allocationManager.end(); it++)
		if(it->owner)
			it->owner->_attribStorage=nullptr;

	// empty allocation maps
	_allocationManager.clear();
#endif
}


// AttribStorage documentation
// note: brief description is with the class declaration
/** \class cd::AttribStorage
 *
 *  It is benefical to store all the vertex attributes in only few AttribStorage
 *  objects because only few OpenGL vertex array objects (VAO) are required,
 *  number of culling and rendering optimizations can be deployed
 *  and the whole scene can be drawn using only few draw calls.
 *
 *  Each AttribStorage object can store vertex attributes of the same format only.
 *  For instance, vertex attributes composed of coordinates and colors are
 *  stored in different AttribStorage than vertex attributes composed of
 *  coordinates, normals and texture coordinates. Format of a particular
 *  attribute must be the same too. For instance, colors stored as RGBA8
 *  are stored in different AttribStorage than colors stored as three floats.
 *
 *  AttribStorage is expected to be used with one graphics context only and
 *  that context is expected to be current when working with the AttribStorage.
 *  The easiest rule is that whenever you are processing your scene graph
 *  and perform any changes that may change the scene geometry, including adding
 *  and removing objects, have your graphics context current.
 *
 *  Constructor require graphics context current to allocate internal VAO
 *  and other OpenGL objects. Destructor requires it to delete these OpenGL objects.
 *  However, there is a difficulty as the graphics context may be destroyed because of
 *  various reasons, including closing of the window or power-saving reasons
 *  on mobile devices. In such cases, the user is expected to call
 *  RenderingContext::contextLost() before any further scene graph processing.
 *  RenderingContext::contextLost() will forward the call to all AttribStorages,
 *  calling their contextLost(). This will clear all internal structures
 *  as if no data would be uploaded to graphics context yet. This could be performed
 *  even without active graphics context, after it was lost or destroyed.
 *  Depending on user choose, he might decide to recreate the graphics
 *  context and reinitialize attribute data, for instance, by reloading
 *  model files, or he might safely start to tear down the application that
 *  is in consistent state after calling RenderingContext::contextLost().
 *
 *  For more details, which methods can be called without active graphics context
 *  refer to the documentation to each of the object's methods.
 *
 *  \sa RenderingContext, Geometry, Geometry::getAttribConfig()
 */
