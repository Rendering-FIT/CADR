#include <CadR/VertexStorage.h>
#include <CadR/Renderer.h>
#include <CadR/StagingBuffer.h>
#include <CadR/VulkanDevice.h>
#include <iostream> // for cerr
#include <memory>

using namespace std;
using namespace CadR;



VertexStorage::VertexStorage(const AttribSizeList& attribSizeList,size_t initialNumVertices)
	: VertexStorage(Renderer::get(),attribSizeList,initialNumVertices)
{
}


VertexStorage::VertexStorage(Renderer* renderer,const AttribSizeList& attribSizeList,size_t initialNumVertices)
	: _allocationManager(uint32_t(initialNumVertices),0)  // set capacity to initialNumVertices, set null object (on index 0) to be of zero size
	, _attribSizeList(attribSizeList)
	, _renderer(renderer)
{
	// no attributes -> do noting
	auto numAttribs=_attribSizeList.size();
	if(numAttribs==0)
		return;

	// resize buffers
	_bufferList.resize(numAttribs,nullptr);
	_memoryList.resize(numAttribs,nullptr);
	if(initialNumVertices==0)
		return;

	// attribute buffers
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.flags=vk::BufferCreateFlags();
	bufferInfo.usage=vk::BufferUsageFlagBits::eVertexBuffer|
	                 vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst;
	bufferInfo.sharingMode=vk::SharingMode::eExclusive;
	bufferInfo.queueFamilyIndexCount=0;
	bufferInfo.pQueueFamilyIndices=nullptr;
	VulkanDevice* device=_renderer->device();
	for(uint32_t i=0; i<numAttribs; i++) {

		uint8_t attribSize=_attribSizeList[i];
		if(attribSize!=0) {

			// create buffer
			bufferInfo.size=attribSize*initialNumVertices;
			vk::Buffer b=device->createBuffer(bufferInfo);
			_bufferList[i]=b;

			// allocate memory
			vk::DeviceMemory m=_renderer->allocateMemory(b,vk::MemoryPropertyFlagBits::eDeviceLocal);
			_memoryList[i]=m;

			// bind memory
			device->bindBufferMemory(
					b,  // buffer
					m,  // memory
					0   // memoryOffset
				);
		}
	}
}


VertexStorage::~VertexStorage()
{
	// free allocations and destroy staging stuff
	_allocationManager.clear();
	_stagingManagerList.clear_and_dispose([](StagingManager* sm){delete sm;});

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
 *  inside VertexStorage.
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
bool VertexStorage::allocData(Drawable* d,unsigned numVertices,unsigned numIndices)
{
	// do we have enough space?
	if(!_vertexAllocationManager.canAllocate(numVertices) ||
	   !_indexAllocationManager.canAllocate(numIndices))
		return false;

	// allocate memory for vertices (inside VertexStorage's preallocated memory or buffers)
	unsigned verticesDataId=_vertexAllocationManager.alloc(numVertices,d);

	// allocate memory for indices (inside VertexStorage's preallocated memory or buffers)
	unsigned indicesDataId;
	if(numIndices==0)
		indicesDataId=0;
	else
		indicesDataId=_indexAllocationManager.alloc(numIndices,d);

	// update Drawable
	if(d->attribStorage()!=NULL)
	{
		cerr<<"Warning: calling VertexStorage::allocData() on Mesh\n"
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
bool VertexStorage::reallocData(Drawable* /*d*/,unsigned /*numVertices*/,unsigned /*numIndices*/,
                                bool /*preserveContent*/)
{
	// Used strategy:
	// - if new arrays are smaller, we keep data in place and free the remaning space
	// - if new arrays are bigger and can be enlarged on the place, we do it
	// - otherwise we try to allocate new place for the data in the same VertexStorage
	// - if we do not succeed in the current VertexStorage, we move the data to
	//   some other VertexStorage
	// - if no VertexStorage accommodate us, we allocate new VertexStorage

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
void VertexStorage::freeData(Drawable* d)
{
	// check whether this attribStorage owns the given Mesh
	if(d->attribStorage()!=this)
	{
		cerr<<"Error: calling VertexStorage::freeData() on Drawable\n"
		      "   that is not managed by this VertexStorage."<<endl;
		return;
	}

	// make sure that there are no draw commands
	// (draw commands has to be freed first, otherwise we can not mark
	// Mesh as empty by setting attribStorage member to NULL)
	if(d->drawCommandDataId()!=0)
	{
		cerr<<"Error: calling VertexStorage::freeData() on Drawable\n"
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


void VertexStorage::uploadVertices(Drawable* /*d*/,std::vector<Buffer>&& /*vertexData*/,size_t /*dstIndex*/)
{
#if 0
   auto& cfg=_attribConfig.configuration();
   unsigned c=unsigned(cfg.attribTypes.size());
   assert(c==attribListSize && "Number of attributes passed in parameters and stored inside "
                               "VertexStorage must match.");
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


StagingBuffer& VertexStorage::createStagingBuffer(Geometry& g,unsigned attribIndex)
{
	if(attribIndex>_bufferList.size())
		throw std::out_of_range("VertexStorage::createStagingBuffer() called with invalid attribIndex.");

	ArrayAllocation& a=allocation(g);
	const unsigned s=_attribSizeList[attribIndex];
	StagingManager& sm=StagingManager::getOrCreate(a,g.vertexDataID(),_stagingManagerList,unsigned(_bufferList.size()));
	return
		sm.createStagingBuffer(
			attribIndex, // stagingBufferIndex
			_renderer,  // renderer
			_bufferList[attribIndex],  // dstBuffer
			a.startIndex*s,  // dstOffset
			a.numItems*s  // size
		);
}


StagingBuffer& VertexStorage::createSubsetStagingBuffer(Geometry& g,unsigned attribIndex,size_t firstVertex,size_t numVertices)
{
	if(attribIndex>_bufferList.size())
		throw std::out_of_range("VertexStorage::createSubsetStagingBuffer() called with invalid attribIndex.");

	ArrayAllocation& a=allocation(g);
	if(firstVertex+numVertices>a.numItems)
		throw std::out_of_range("VertexStorage::createSubsetStagingBuffer() called with firstVertex and numVertices that specify the range hitting outside of Geometry preallocated space.");

	const unsigned s=_attribSizeList[attribIndex];
	StagingManager& sm=StagingManager::getOrCreateForSubsetUpdates(a,g.vertexDataID(),_stagingManagerList);
	return
		sm.createStagingBuffer(
			_renderer,  // renderer
			_bufferList[attribIndex],  // dstBuffer
			(a.startIndex+firstVertex)*s,  // dstOffset
			numVertices*s  // size
		);
}


vector<StagingBuffer*> VertexStorage::createStagingBuffers(Geometry& g)
{
	vector<StagingBuffer*> r;
	r.reserve(_bufferList.size());

	ArrayAllocation& a=allocation(g);
	StagingManager& sm=StagingManager::getOrCreate(a,g.vertexDataID(),_stagingManagerList,unsigned(_bufferList.size()));

	for(size_t i=0,e=_bufferList.size(); i<e; i++) {
		vk::Buffer b=_bufferList[i];
		const unsigned s=_attribSizeList[i];
		StagingBuffer& sb=
			sm.createStagingBuffer(
				unsigned(i),  // stagingBufferIndex
				_renderer,  // renderer
				b,  // dstBuffer
				a.startIndex*s,  // dstOffset
				a.numItems*s  // size
			);
		r.emplace_back(&sb);
	}
	return r;
}


vector<StagingBuffer*> VertexStorage::createSubsetStagingBuffers(Geometry& g,size_t firstVertex,size_t numVertices)
{
	vector<StagingBuffer*> r;
	r.reserve(_bufferList.size());

	ArrayAllocation& a=allocation(g);
	if(firstVertex+numVertices>a.numItems)
		throw std::out_of_range("VertexStorage::createSubsetStagingBuffers(): Parameter firstVertex and numVertices define range that is not completely inside allocated space of Geometry.");
	StagingManager& sm=StagingManager::getOrCreateForSubsetUpdates(a,g.vertexDataID(),_stagingManagerList);

	for(size_t i=0,e=_bufferList.size(); i<e; i++) {
		vk::Buffer b=_bufferList[i];
		const unsigned s=_attribSizeList[i];
		StagingBuffer& sb=
			sm.createStagingBuffer(
				_renderer,  // renderer
				b,  // dstBuffer
				(a.startIndex+firstVertex)*s,  // dstOffset
				numVertices*s  // size
			);
		r.emplace_back(&sb);
	}
	return r;
}


void VertexStorage::upload(Geometry& g,unsigned attribIndex,const std::vector<uint8_t>& attribData)
{
	// attribIndex bound check
	if(attribIndex>_bufferList.size())
		throw std::out_of_range("VertexStorage::upload() called with invalid attribIndex.");

	// create StagingBuffer and submit it
	StagingBuffer& sb=createStagingBuffer(g,attribIndex);
	if(attribData.size()!=sb.size())
		throw std::out_of_range("VertexStorage::upload(): size of attribData must match Geometry allocated space.");
	memcpy(sb.data(),attribData.data(),attribData.size());
}


void VertexStorage::uploadSubset(Geometry& g,unsigned attribIndex,const std::vector<uint8_t>& attribData,size_t firstVertex)
{
	// attribIndex bound check
	if(attribIndex>_bufferList.size())
		throw std::out_of_range("VertexStorage::uploadAttrib() called with invalid attribIndex.");

	if(attribData.empty()) return;

	// create StagingBuffer and submit it
	size_t numVertices=attribData.size()/_attribSizeList[attribIndex];
	StagingBuffer& sb=createSubsetStagingBuffer(g,attribIndex,firstVertex,numVertices);
	memcpy(sb.data(),attribData.data(),attribData.size());
}


void VertexStorage::upload(Geometry& g,const vector<vector<uint8_t>>& vertexData)
{
	// check parameters validity
	if(vertexData.size()!=_bufferList.size())
		throw std::out_of_range("VertexStorage::uploadAttribs() called with invalid vertexData.");
	if(vertexData.size()==0)
		return;
	size_t numVertices=vertexData[0].size()/_attribSizeList[0];
	for(size_t i=1,e=vertexData.size(); i<e; i++)
		if(vertexData[i].size()!=numVertices*_attribSizeList[i])
			throw std::out_of_range("VertexStorage::uploadAttribs() called with invalid vertexData.");
	if(numVertices==0) return;

	// create StagingBuffers and submit them
	for(size_t i=0,e=vertexData.size(); i<e; i++) {
		StagingBuffer& sb=createStagingBuffer(g,unsigned(i));
		memcpy(sb.data(),vertexData[i].data(),sb.size());
	}
}


void VertexStorage::uploadSubset(Geometry& g,const vector<vector<uint8_t>>& vertexData,size_t firstVertex)
{
	// check parameters validity
	if(vertexData.size()!=_bufferList.size())
		throw std::out_of_range("VertexStorage::uploadAttribs() called with invalid vertexData.");
	if(vertexData.size()==0)
		return;
	size_t numVertices=vertexData[0].size()/_attribSizeList[0];
	for(size_t i=1,e=vertexData.size(); i<e; i++)
		if(vertexData[i].size()!=numVertices*_attribSizeList[i])
			throw std::out_of_range("VertexStorage::uploadAttribs() called with invalid vertexData.");
	if(numVertices==0) return;

	// create StagingBuffers and submit them
	for(size_t i=0,e=vertexData.size(); i<e; i++) {
		StagingBuffer& sb=createSubsetStagingBuffer(g,unsigned(i),firstVertex,numVertices);
		memcpy(sb.data(),vertexData[i].data(),sb.size());
	}
}


void VertexStorage::reallocStorage(size_t newNumVertices)
{
	auto numAttribs=this->numAttribs();
	if(numAttribs==0) {
		_allocationManager.setCapacity(newNumVertices);
		return;
	}

	// store safely new buffers
	struct NewBuffersAndMemory {
		vector<vk::Buffer> bufferList;
		vector<vk::DeviceMemory> memoryList;
		VulkanDevice* device;
		NewBuffersAndMemory(VulkanDevice* d) : device(d)  {}
		~NewBuffersAndMemory()  { for(vk::Buffer b : bufferList) device->destroy(b); for(vk::DeviceMemory m : memoryList) device->free(m); }
	};
	VulkanDevice* device=_renderer->device();
	NewBuffersAndMemory n(device);

	// fill lists with nulls
	n.bufferList.resize(numAttribs,nullptr);
	n.memoryList.resize(numAttribs,nullptr);
	if(newNumVertices==0) {
		_allocationManager.setCapacity(0);
		_bufferList.swap(n.bufferList);
		_memoryList.swap(n.memoryList);
		return;
	}

	// allocate command buffer
	auto commandPool=
		device->createCommandPoolUnique(
			vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlagBits::eTransient,  // flags
				_renderer->graphicsQueueFamily()  // queueFamilyIndex
			)
		);
	vk::CommandBuffer cb=
		device->allocateCommandBuffers(
			vk::CommandBufferAllocateInfo(
				commandPool.get(),  // commandPool
				vk::CommandBufferLevel::ePrimary,  // level
				1  // commandBufferCount
			)
		)[0];

	// start recording
	device->beginCommandBuffer(
		cb,  // commandBuffer
		vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,  // flags
			nullptr  // pInheritanceInfo
		)
	);

	// create new buffers and deviceMemories
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.flags=vk::BufferCreateFlags();
	bufferInfo.usage=vk::BufferUsageFlagBits::eVertexBuffer|
	                 vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst;
	bufferInfo.sharingMode=vk::SharingMode::eExclusive;
	bufferInfo.queueFamilyIndexCount=0;
	bufferInfo.pQueueFamilyIndices=nullptr;
	for(uint32_t i=0; i<numAttribs; i++) {

		uint8_t attribSize=_attribSizeList[i];
		if(attribSize!=0) {

			// create new buffer
			bufferInfo.size=attribSize*newNumVertices;
			vk::Buffer b=device->createBuffer(bufferInfo);
			_bufferList[i]=b;

			// allocate memory
			vk::DeviceMemory m=_renderer->allocateMemory(b,vk::MemoryPropertyFlagBits::eDeviceLocal);
			_memoryList[i]=m;

			// bind memory
			device->bindBufferMemory(
					b,  // buffer
					m,  // memory
					0   // memoryOffset
				);

			// copy content
			size_t copySize=min(bufferInfo.size,attribSize*size_t(_allocationManager.capacity()));
			device->cmdCopyBuffer(cb,_bufferList[i],b,vk::BufferCopy(0,0,copySize));
		}
	}

	// submit command buffer
	device->endCommandBuffer(cb);
	auto fence=device->createFenceUnique(vk::FenceCreateInfo{vk::FenceCreateFlags()});
	device->queueSubmit(
		_renderer->graphicsQueue(),  // queue
		vk::SubmitInfo(  // submits (vk::ArrayProxy)
			0,nullptr,nullptr,  // waitSemaphoreCount,pWaitSemaphores,pWaitDstStageMask
			1,&cb,  // commandBufferCount,pCommandBuffers
			0,nullptr  // signalSemaphoreCount,pSignalSemaphores
		),
		fence.get()  // fence
	);

	// wait for work to complete
	vk::Result r=device->waitForFences(
		fence.get(),   // fences (vk::ArrayProxy)
		VK_TRUE,       // waitAll
		uint64_t(3e9)  // timeout (3s)
	);
	if(r==vk::Result::eTimeout)
		throw std::runtime_error("GPU timeout. Task is probably hanging.");

	// commit changes
	_allocationManager.setCapacity(newNumVertices);
	_bufferList.swap(n.bufferList);
	_memoryList.swap(n.memoryList);
}


#if 0
void VertexStorage::render(const std::vector<RenderingCommandData>& renderingDataList)
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


void VertexStorage::cancelAllAllocations()
{
#if 0
	// break all Mesh references to this VertexStorage
	for(auto it=_allocationManager.begin(); it!=_allocationManager.end(); it++)
		if(it->owner)
			it->owner->_attribStorage=nullptr;

	// empty allocation maps
	_allocationManager.clear();
#endif
}


// VertexStorage documentation
// note: brief description is with the class declaration
/** \class cd::VertexStorage
 *
 *  It is benefical to store all the vertex attributes in only few VertexStorage
 *  objects because only few OpenGL vertex array objects (VAO) are required,
 *  number of culling and rendering optimizations can be deployed
 *  and the whole scene can be drawn using only few draw calls.
 *
 *  Each VertexStorage object can store vertex attributes of the same format only.
 *  For instance, vertex attributes composed of coordinates and colors are
 *  stored in different VertexStorage than vertex attributes composed of
 *  coordinates, normals and texture coordinates. Format of a particular
 *  attribute must be the same too. For instance, colors stored as RGBA8
 *  are stored in different VertexStorage than colors stored as three floats.
 *
 *  VertexStorage is expected to be used with one graphics context only and
 *  that context is expected to be current when working with the VertexStorage.
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
 *  RenderingContext::contextLost() will forward the call to all VertexStorages,
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
