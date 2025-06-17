#ifndef CADR_TRANSFER_RESOURCES_HEADER
# define CADR_TRANSFER_RESOURCES_HEADER

#include <functional>

namespace CadR {


/** TransferResources class is used to release resources after particular transfer or stransfers
 *  from StagingMemory to DataMemory are completed. It holds two iterators for each DataMemory
 *  referenced by this TransferResources. These are fed to DataMemory::uploadDone() function\
 *  that does the actual resource release.
 */
class CADR_EXPORT TransferResources {
protected:
	std::function<void(void)> _releaseFunction;
public:
	inline TransferResources();
	template<typename F, typename... Args>
	inline TransferResources(F&& f, Args&&... args);
	inline void release();
	inline ~TransferResources();

	TransferResources(const TransferResources&) = delete;
	inline TransferResources(TransferResources&& other);
	TransferResources& operator=(const TransferResources&) = delete;
	inline TransferResources& operator=(TransferResources&& other);
};


}

#endif


// inline functions
#if !defined(CADR_TRANSFER_RESOURCES_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_TRANSFER_RESOURCES_INLINE_FUNCTIONS
# include <utility>
namespace CadR {

inline TransferResources::TransferResources()  : _releaseFunction(nullptr) {}
template<typename F, typename... Args>
inline TransferResources::TransferResources(F&& f, Args&&... args)  : _releaseFunction(std::bind(f, args...)) {}
inline void TransferResources::release()  { if(!_releaseFunction) return; _releaseFunction(); _releaseFunction = nullptr; }
inline TransferResources::~TransferResources()  { release(); }

inline TransferResources::TransferResources(TransferResources&& other)  : _releaseFunction(std::move(other._releaseFunction)) { other._releaseFunction = nullptr; }
inline TransferResources& TransferResources::operator=(TransferResources&& rhs)  { _releaseFunction = std::move(rhs._releaseFunction); rhs._releaseFunction = nullptr; return *this; }

}
#endif
