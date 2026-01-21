#ifndef CADR_MATRIX_LIST_HEADER
# define CADR_MATRIX_LIST_HEADER

# ifndef CADR_NO_INLINE_FUNCTIONS
#  define CADR_NO_INLINE_FUNCTIONS
#  include <CadR/DataAllocation.h>
#  undef CADR_NO_INLINE_FUNCTIONS
# else
#  include <CadR/DataAllocation.h>
# endif
# include <glm/mat4x4.hpp>
# include <vector>

namespace CadR {

class Renderer;



class CADR_EXPORT MatrixList {
protected:
	DataAllocation _matrixList;
	inline void initZeroField(void* p, size_t numMatrices);
public:

	inline MatrixList(Renderer& r);
	inline MatrixList(MatrixList&& other) = default;

	inline void setMatrices(const glm::mat4& matrix);
	inline void setMatrices(const std::vector<glm::mat4>& matrices);
	inline void setMatrices(const glm::mat4* matrices, size_t numMatrices);
	inline glm::mat4* editNewContent(size_t numMatrices);

	inline uint64_t handle() const;

};


}

#endif


// inline functions
#if !defined(CADR_MATRIX_LIST_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_MATRIX_LIST_INLINE_FUNCTIONS
# include <CadR/Renderer.h>
namespace CadR {

inline void MatrixList::initZeroField(void* p, size_t numMatrices)  { reinterpret_cast<uint32_t*>(p)[0]=uint32_t(numMatrices); reinterpret_cast<uint32_t*>(p)[1]=uint32_t(numMatrices); for(unsigned i=1; i<8; i++) reinterpret_cast<uint64_t*>(p)[i]=0; }
inline MatrixList::MatrixList(Renderer& r)  : _matrixList(r.dataStorage()) {}
inline void MatrixList::setMatrices(const glm::mat4& matrix)  { StagingData sd=_matrixList.alloc(sizeof(matrix)*2); glm::mat4* m=sd.data<glm::mat4>(); initZeroField(m, 1); m[1]=matrix; }
inline void MatrixList::setMatrices(const std::vector<glm::mat4>& matrices)  { size_t n=matrices.size(); size_t s=sizeof(glm::mat4)*n; StagingData sd=_matrixList.alloc(s+sizeof(glm::mat4)); glm::mat4* m=sd.data<glm::mat4>(); initZeroField(m, n); memcpy(m+1, matrices.data(), s); }
inline void MatrixList::setMatrices(const glm::mat4* matrices, size_t numMatrices)  { size_t s=sizeof(glm::mat4)*numMatrices; StagingData sd=_matrixList.alloc(s+sizeof(glm::mat4)); glm::mat4* m=sd.data<glm::mat4>(); initZeroField(m, numMatrices); memcpy(m+1, matrices, s); }
inline glm::mat4* MatrixList::editNewContent(size_t numMatrices)  { size_t s=sizeof(glm::mat4)*numMatrices; StagingData sd=_matrixList.alloc(s+sizeof(glm::mat4)); glm::mat4* m=sd.data<glm::mat4>(); initZeroField(m, numMatrices); return m+1; }
inline uint64_t MatrixList::handle() const  { return _matrixList.handle(); }

}
#endif
