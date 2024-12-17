#ifndef CADR_STAGING_MANAGER_HEADER
# define CADR_STAGING_MANAGER_HEADER

# ifndef CADR_NO_INLINE_FUNCTIONS
#  define CADR_NO_INLINE_FUNCTIONS
#  include <CadR/StagingMemory.h>
#  undef CADR_NO_INLINE_FUNCTIONS
# else
#  include <CadR/StagingMemory.h>
# endif
# include <boost/intrusive/list.hpp>

namespace CadR {

class Renderer;


class CADR_EXPORT StagingManager {
protected:
	using StagingMemoryList =
		boost::intrusive::list<
			StagingMemory,
			boost::intrusive::member_hook<
				StagingMemory,
				boost::intrusive::list_member_hook<
					boost::intrusive::link_mode<boost::intrusive::auto_unlink>>,
				&StagingMemory::_stagingMemoryListHook>,
			boost::intrusive::constant_time_size<false>
		>;
	struct Disposer {
		Disposer& operator()(StagingMemory* sm)  { delete sm; return *this; }
	};
	StagingMemoryList _smallMemoryInUseList;
	StagingMemoryList _smallMemoryAvailableList;
	StagingMemoryList _mediumMemoryInUseList;
	StagingMemoryList _mediumMemoryAvailableList;
	StagingMemoryList _largeMemoryInUseList;
	StagingMemoryList _largeMemoryAvailableList;
	StagingMemoryList _superSizeMemoryInUseList;
	StagingMemoryList _superSizeMemoryAvailableList;
	Renderer* _renderer;
	StagingMemory& reuseOrAllocStagingMemory(StagingMemoryList& availableList, StagingMemoryList& inUseList, size_t size);
public:

	StagingManager(Renderer& r);
	~StagingManager() noexcept;
	void cleanUp() noexcept;

	StagingMemory& reuseOrAllocSmallStagingMemory();
	StagingMemory& reuseOrAllocMediumStagingMemory();
	StagingMemory& reuseOrAllocLargeStagingMemory();
	StagingMemory& reuseOrAllocSuperSizeStagingMemory(size_t size);
	void freeOrRecycleStagingMemory(StagingMemory& sm) noexcept;

	Renderer& renderer() const;
	bool isEmpty() const;

};


}

#endif


// inline methods
#if !defined(CADR_STAGING_MANAGER_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_STAGING_MANAGER_INLINE_FUNCTIONS
# define CADR_NO_INLINE_FUNCTIONS
# include <CadR/Renderer.h>
# undef CADR_NO_INLINE_FUNCTIONS
namespace CadR {

inline StagingManager::StagingManager(Renderer& r)  : _renderer(&r) {}
inline StagingManager::~StagingManager() noexcept  { cleanUp(); }
inline StagingMemory& StagingManager::reuseOrAllocSmallStagingMemory()  { return reuseOrAllocStagingMemory(_smallMemoryAvailableList, _smallMemoryInUseList, Renderer::smallMemorySize); }
inline StagingMemory& StagingManager::reuseOrAllocMediumStagingMemory()  { return reuseOrAllocStagingMemory(_mediumMemoryAvailableList, _mediumMemoryInUseList, Renderer::mediumMemorySize); }
inline StagingMemory& StagingManager::reuseOrAllocLargeStagingMemory()  { return reuseOrAllocStagingMemory(_largeMemoryAvailableList, _largeMemoryInUseList, Renderer::largeMemorySize); }

inline Renderer& StagingManager::renderer() const  { return *_renderer; }
inline bool StagingManager::isEmpty() const  { return _smallMemoryInUseList.empty() && _smallMemoryAvailableList.empty() && _mediumMemoryInUseList.empty() && _mediumMemoryAvailableList.empty() &&
	_largeMemoryInUseList.empty() && _largeMemoryAvailableList.empty() && _superSizeMemoryInUseList.empty() && _superSizeMemoryAvailableList.empty(); }

}
#endif
