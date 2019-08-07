#include <iostream> // for cerr
#include <memory>
#include <CadR/AttribStorage.h>
#include <CadR/Renderer.h>

using namespace std;
using namespace CadR;



AttribStorage::AttribStorage(Renderer* renderer,const AttribConfig& attribConfig)
	: _allocationManager(0,0)  // zero capacity, zero-sized object on index 0
	, _attribConfig(attribConfig)
	, _renderer(renderer)
{
#if 0
	//_renderer->onAttribStorageInit(this);

	// create buffers
	_bufferList.reserve(_attribConfig.numAttribs());
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.flags=vk::BufferCreateFlags();
	bufferInfo.usage=vk::BufferUsageFlagBits::eVertexBuffer;
	bufferInfo.sharingMode=vk::SharingMode::eExclusive;
	bufferInfo.queueFamilyIndexCount=0;
	bufferInfo.pQueueFamilyIndices=nullptr;
	vk::Device* device=_renderer->device();
	for(uint8_t size : _attribConfig.sizeList())
	{
		vk::Buffer b;
		if(size!=0) {
			bufferInfo.size=size*1024;
			b=device->createBuffer(bufferInfo);
		} else
			b=nullptr;
		_bufferList.push_back(b);
	}

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

   // append transformation matrix to VAO
#if 0
   if(_renderingContext->getUseARBShaderDrawParameters()==false) {
      _va->bind();
      _renderingContext->matrixStorage()->buffer()->bind(GL_ARRAY_BUFFER);
      const GLuint index=12;
      auto& gl=_va->getContext();
      gl.glEnableVertexAttribArray(index+0);
      gl.glEnableVertexAttribArray(index+1);
      gl.glEnableVertexAttribArray(index+2);
      gl.glEnableVertexAttribArray(index+3);
      gl.glVertexAttribPointer(index+0,4,GL_FLOAT,GL_FALSE,int(sizeof(float)*16),(void*)(sizeof(float)*0));
      gl.glVertexAttribPointer(index+1,4,GL_FLOAT,GL_FALSE,int(sizeof(float)*16),(void*)(sizeof(float)*4));
      gl.glVertexAttribPointer(index+2,4,GL_FLOAT,GL_FALSE,int(sizeof(float)*16),(void*)(sizeof(float)*8));
      gl.glVertexAttribPointer(index+3,4,GL_FLOAT,GL_FALSE,int(sizeof(float)*16),(void*)(sizeof(float)*12));
      gl.glVertexAttribDivisor(index+0,1);
      gl.glVertexAttribDivisor(index+1,1);
      gl.glVertexAttribDivisor(index+2,1);
      gl.glVertexAttribDivisor(index+3,1);
      _va->unbind();
   }
#endif
}


AttribStorage::~AttribStorage()
{
	// invalidate all allocations and clean up
	cancelAllAllocations();
	//_renderer->onAttribStorageRelease(this);
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


void AttribStorage::uploadAttrib(Drawable* /*d*/,unsigned /*attribIndex*/,Buffer&& /*attribData*/,size_t /*dstIndex*/)
{
	assert(0 && "Not implemented yet.");
}


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
 *  \sa RenderingContext, Mesh, Mesh::getAttribConfig()
 */
