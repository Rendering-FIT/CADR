#pragma once

#include <vulkan/vulkan.hpp>
#include <CadR/Export.h>
#include <CadR/AttribConfig.h>
#include <CadR/AllocationManagers.h>

namespace CadR {

class Mesh;
class Renderer;


/** \brief AttribStorage class provides GPU storage for vertex attributes
 *  of the scene geometry. All the data are stored in vk::Buffers.
 *  Any number of Mesh objects can be stored in a single AttribStorage
 *  as long as all the stored Meshes have the same attribute configuration (see AttribConfig)
 *  and as long as there is the free space in the buffers.
 */
class CADR_EXPORT AttribStorage final {
protected:

	ArrayAllocationManager<Mesh> _allocationManager;  ///< Allocation manager for attribute data.
	AttribConfig _attribConfig;  ///< Configuration and formats of OpenGL attributes stored in this AttribStorage.
	Renderer* _renderer;  ///< Rendering context associated with the AttribStorage.

	std::vector<vk::Buffer> _bufferList;

public:

	AttribStorage() = delete;
	AttribStorage(Renderer* r,const AttribConfig& c);
	~AttribStorage();

	AttribStorage(const AttribStorage&) = delete;
	AttribStorage(AttribStorage&&) = default;
	AttribStorage& operator=(const AttribStorage&) = delete;
	AttribStorage& operator=(AttribStorage&&) = default;

	void allocAttribs(Mesh& m,size_t num);
	void reallocAttribs(Mesh& m,size_t num);
	void freeAttribs(Mesh& m);

	void uploadAttribs(Mesh& d,std::vector<std::vector<uint8_t>>&& vertexData,size_t dstIndex=0);
	void uploadAttrib(Mesh& d,unsigned attribIndex,std::vector<uint8_t>&& attribData,size_t dstIndex=0);

	const ArrayAllocation<Mesh>& attribAllocation(unsigned id) const; ///< Returns the attribute allocation for particular id.
	ArrayAllocation<Mesh>& attribAllocation(unsigned id);  ///< Returns the attribute allocation for particular id. Modify the returned data only with caution.
	const ArrayAllocationManager<Mesh>& allocationManager() const;  ///< Returns the allocation manager.
	ArrayAllocationManager<Mesh>& allocationManager();  ///< Returns the allocation manager.

	const AttribConfig& attribConfig() const;
	Renderer* renderer() const;

	inline const std::vector<vk::Buffer>& bufferList() const;
	inline vk::Buffer buffer(unsigned index) const;
	inline unsigned numBuffers() const;

	void render();
	void cancelAllAllocations();

};


}



// inline and template methods
#include <CadR/Mesh.h>
namespace CadR {

inline void AttribStorage::allocAttribs(Mesh& m,size_t num)  { m.allocAttribs(num); }
inline void AttribStorage::reallocAttribs(Mesh& m,size_t num)  { m.reallocAttribs(num); }
inline void AttribStorage::freeAttribs(Mesh& m)  { m.freeAttribs(); }
inline const ArrayAllocation<Mesh>& AttribStorage::attribAllocation(unsigned id) const  { return _allocationManager[id]; }
inline ArrayAllocation<Mesh>& AttribStorage::attribAllocation(unsigned id)  { return _allocationManager[id]; }
inline const ArrayAllocationManager<Mesh>& AttribStorage::allocationManager() const  { return _allocationManager; }
inline ArrayAllocationManager<Mesh>& AttribStorage::allocationManager()  { return _allocationManager; }
inline const AttribConfig& AttribStorage::attribConfig() const  { return _attribConfig; }
inline Renderer* AttribStorage::renderer() const  { return _renderer; }
inline const std::vector<vk::Buffer>& AttribStorage::bufferList() const  { return _bufferList; }
inline vk::Buffer AttribStorage::buffer(unsigned index) const  { return _bufferList[index]; }
inline unsigned AttribStorage::numBuffers() const  { return unsigned(_bufferList.size()); }

}
