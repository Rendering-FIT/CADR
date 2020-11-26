#pragma once

#include <CadR/StagingBuffer.h>
#include <boost/intrusive/list.hpp>

namespace CadR {

struct ArrayAllocation;


class CADR_EXPORT StagingManager {
protected:
	int _state;
	uint32_t _allocationID;
	union {
		StagingBuffer _stagingBuffer;
		std::vector<StagingBuffer> _stagingBufferList;
	};
	void setSubsetUpdateMode(bool v);
	bool vectorAllocated() const;
	void setVectorAllocated(bool v);
public:
	boost::intrusive::list_member_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink>
	> _stagingManagerListHook;  ///< List hook of Renderer::_drawableList. Geometry::_drawableList contains all Drawables that render the Geometry.
	typedef boost::intrusive::list<
		StagingManager,
		boost::intrusive::member_hook<
			StagingManager,
			boost::intrusive::list_member_hook<
				boost::intrusive::link_mode<boost::intrusive::auto_unlink>>,
			&StagingManager::_stagingManagerListHook>,
		boost::intrusive::constant_time_size<false>
	> StagingManagerList;
public:

	static StagingManager& getOrCreate(ArrayAllocation& a,uint32_t allocationID,StagingManagerList& l,unsigned numBuffers=1);
	static StagingManager& getOrCreateForSubsetUpdates(ArrayAllocation& a,uint32_t allocationID,StagingManagerList& l);

	StagingManager() = delete;
	StagingManager(bool subsetUpdateMode,uint32_t allocationID,StagingManagerList& l,unsigned numBuffers=1) noexcept;
	~StagingManager() noexcept;
	void destroy();

	bool subsetUpdateMode() const;
	uint32_t allocationID() const;

	StagingBuffer& createStagingBuffer(Renderer* renderer,vk::Buffer dstBuffer,size_t dstOffset,size_t size);
	StagingBuffer& createStagingBuffer(unsigned stagingBufferIndex,Renderer* renderer,vk::Buffer dstBuffer,size_t dstOffset,size_t size);
	StagingBuffer& getStagingBuffer();
	StagingBuffer& getStagingBuffer(unsigned index);
	unsigned getNumStagingBuffers();
	std::vector<StagingBuffer>* getStagingBufferList();

	void record(vk::CommandBuffer commandBuffer);

};


typedef StagingManager::StagingManagerList StagingManagerList;


}


// inline methods
#include <CadR/AllocationManagers.h>
namespace CadR {
inline bool StagingManager::subsetUpdateMode() const  { return _state&0x1;}
inline void StagingManager::setSubsetUpdateMode(bool v)  { if(v) _state|=1; else _state&=~0x1; }
inline bool StagingManager::vectorAllocated() const  { return _state&0x2; }
inline void StagingManager::setVectorAllocated(bool v)  { if(v) _state|=2; else _state&=~0x2; }
inline StagingManager& StagingManager::getOrCreate(ArrayAllocation& a,uint32_t allocationID,StagingManagerList& l,unsigned numBuffers)  { if(a.stagingManager) { if(a.stagingManager->subsetUpdateMode()) throw std::runtime_error("StagingManager::getOrCreate() called for allocation that has already StagingManager but in subsetUpdateMode."); } else a.stagingManager=new StagingManager(false,allocationID,l,numBuffers); return *a.stagingManager; }
inline StagingManager& StagingManager::getOrCreateForSubsetUpdates(ArrayAllocation& a,uint32_t allocationID,StagingManagerList& l)  { if(a.stagingManager) { if(!a.stagingManager->subsetUpdateMode()) throw std::runtime_error("StagingManager::getOrCreateForSubsetUpdates() called for allocation that has already StagingManager but not in subsetUpdateMode."); } else a.stagingManager=new StagingManager(true,allocationID,l); return *a.stagingManager; }
inline StagingManager::~StagingManager()  { destroy(); }
inline void StagingManager::destroy()
{
	if(!vectorAllocated()) {
		if(_stagingBuffer._renderer!=nullptr) {
			_stagingBuffer.~StagingBuffer();
			_stagingBuffer._renderer=nullptr;
		}
	}
	else {
		_stagingBufferList.~vector<StagingBuffer>();
		setVectorAllocated(false);
		_stagingBuffer._renderer=nullptr;
	}
}
inline uint32_t StagingManager::allocationID() const  { return _allocationID; }
inline StagingBuffer& StagingManager::getStagingBuffer()  { if(vectorAllocated()) return _stagingBufferList[0]; else return _stagingBuffer; }
inline StagingBuffer& StagingManager::getStagingBuffer(unsigned index)  { assert(index>0 && vectorAllocated() && index<_stagingBufferList.size()); if(vectorAllocated()) return _stagingBufferList[index]; else return _stagingBuffer; }
inline unsigned StagingManager::getNumStagingBuffers()  { if(vectorAllocated()) return unsigned(_stagingBufferList.size()); return _stagingBuffer._renderer==nullptr?0:1; }
inline std::vector<StagingBuffer>* StagingManager::getStagingBufferList()  { if(vectorAllocated()) return &_stagingBufferList; else return nullptr; }


}
