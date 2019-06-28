#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>
#include <CADR/Export.h>
#include <CADR/AttribConfig.h>
#include <CADR/AllocationManagers.h>

namespace cd {

struct Buffer;
class Drawable;
class Renderer;


/** \brief AttribStorage class provides GPU storage for vertex attributes
 *  and vertex indices of the scene geometry. All the data are stored in
 *  vk::Buffers. Any number of Drawable objects can be stored
 *  in a single AttribStorage as long as they have the same attribute
 *  configuration (see AttribConfig) and as long as there is space in the
 *  preallocated buffers.
 */
class CADR_EXPORT AttribStorage {
protected:

	ArrayAllocationManager<Drawable> _vertexAllocationManager;  ///< Allocation manager for arrays of vertices.
	ArrayAllocationManager<Drawable> _indexAllocationManager;   ///< Allocation manager for arrays of indices.
	AttribConfig _attribConfig;  ///< Configuration and formats of OpenGL attributes stored in this AttribStorage.
	Renderer* _renderer;  ///< Rendering context that the AttribStorage lives in. Rendering context is required particularly from object destructor.

	std::vector<vk::Buffer> _bufferList;
	vk::Buffer _indexBuffer;

public:

	static inline AttribStorage* create(Renderer* r,const AttribConfig& c);
	static inline std::shared_ptr<AttribStorage> make_shared(Renderer* r,const AttribConfig& c);
	AttribStorage() = delete;
	AttribStorage(Renderer* r,const AttribConfig& c);
	virtual ~AttribStorage();

	virtual void bind() const;

	virtual bool allocData(Drawable* d,unsigned numVertices,unsigned numIndices);
	virtual bool reallocData(Drawable* d,unsigned numVertices,unsigned numIndices,
	                         bool preserveContent=true);
	virtual void freeData(Drawable* d);

	void uploadVertices(Drawable* d,std::vector<Buffer>&& vertexData,size_t dstIndex=0);
	void uploadAttrib(Drawable* d,unsigned attribIndex,Buffer&& attribData,size_t dstIndex=0);
	void uploadIndices(Drawable* d,std::vector<uint32_t>&& indexData,size_t dstIndex=0);

	inline const ArrayAllocation<Drawable>& vertexArrayAllocation(unsigned id) const; ///< Returns vertex array allocation for particular id.
	inline const ArrayAllocation<Drawable>& indexArrayAllocation(unsigned id) const;  ///< Returns index array allocation for particular id.
	inline ArrayAllocation<Drawable>& vertexArrayAllocation(unsigned id);  ///< Returns vertex array allocation for particular id. When modifying returned data, the care must be taken to not break internal data consistency.
	inline ArrayAllocation<Drawable>& indexArrayAllocation(unsigned id);   ///< Returns index array allocation for particular id. When modifying returned data, the care must be taken to not break internal data consistency.

	inline ArrayAllocationManager<Drawable>& vertexAllocationManager();
	inline ArrayAllocationManager<Drawable>& indexAllocationManager();
	inline const ArrayAllocationManager<Drawable>& vertexAllocationManager() const;
	inline const ArrayAllocationManager<Drawable>& indexAllocationManager() const;

	inline const AttribConfig& attribConfig() const;
	inline Renderer* renderer() const;

	inline const std::vector<vk::Buffer>& bufferList() const;
	inline vk::Buffer buffer(unsigned index) const;
	inline unsigned numBuffers() const;
	inline vk::Buffer indexBuffer() const;

	virtual void render();
	virtual void cancelAllAllocations();

};


}



// inline and template methods
//
// note: they need their own includes that can not be placed on the beginning of this file
//       as there are circular header includes and the classes need to be defined before
//       inline methods to avoid incomplete type compiler error
#include <CADR/Factory.h>
namespace cd {

inline AttribStorage* AttribStorage::create(Renderer* r,const AttribConfig& c)  { return Factory::get()->createAttribStorage(r,c); }
inline std::shared_ptr<AttribStorage> AttribStorage::make_shared(Renderer* r,const AttribConfig& c)  { return Factory::get()->makeAttribStorage(r,c); }
inline const ArrayAllocation<Drawable>& AttribStorage::vertexArrayAllocation(unsigned id) const  { return _vertexAllocationManager[id]; }
inline const ArrayAllocation<Drawable>& AttribStorage::indexArrayAllocation(unsigned id) const  { return _indexAllocationManager[id]; }
inline ArrayAllocation<Drawable>& AttribStorage::vertexArrayAllocation(unsigned id)  { return _vertexAllocationManager[id]; }
inline ArrayAllocation<Drawable>& AttribStorage::indexArrayAllocation(unsigned id)  { return _indexAllocationManager[id]; }
inline ArrayAllocationManager<Drawable>& AttribStorage::vertexAllocationManager()  { return _vertexAllocationManager; }
inline ArrayAllocationManager<Drawable>& AttribStorage::indexAllocationManager()  { return _indexAllocationManager; }
inline const ArrayAllocationManager<Drawable>& AttribStorage::vertexAllocationManager() const  { return _vertexAllocationManager; }
inline const ArrayAllocationManager<Drawable>& AttribStorage::indexAllocationManager() const  { return _indexAllocationManager; }
inline const AttribConfig& AttribStorage::attribConfig() const  { return _attribConfig; }
inline Renderer* AttribStorage::renderer() const  { return _renderer; }
inline const std::vector<vk::Buffer>& AttribStorage::bufferList() const  { return _bufferList; }
inline vk::Buffer AttribStorage::buffer(unsigned index) const  { return _bufferList[index]; }
inline unsigned AttribStorage::numBuffers() const  { return unsigned(_bufferList.size()); }
inline vk::Buffer AttribStorage::indexBuffer() const  { return _indexBuffer; }

}
