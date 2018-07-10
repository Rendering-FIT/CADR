#pragma once

#include <memory>

namespace ri {


struct Buffer {

	std::unique_ptr<uint8_t[]> data;
	size_t size;

	inline Buffer() : size(0) {}
	inline Buffer(void* p,size_t s) : data(reinterpret_cast<uint8_t*>(p)), size(s) {}
	inline Buffer(Buffer&&) = default;
	inline Buffer& operator=(Buffer&&) = default;
	Buffer(Buffer&) = delete;
	Buffer& operator=(Buffer&) = delete;

	static inline Buffer createByAlloc(size_t s);
	template<typename T> static inline Buffer createByAlloc(size_t n);
	static inline Buffer createByCopy(const void* src,size_t n);
	template<typename T,std::size_t N> static inline Buffer createByCopy(const std::array<T,N>& src);
	template<typename T> static inline Buffer createByCopy(const std::vector<T>& src);

	inline void* get()  { return data.get(); }
	template<typename T> inline T* get()  { return reinterpret_cast<T*>(data.get()); }
	inline void set(void* p,size_t s)  { data.reset(static_cast<uint8_t*>(p)); size=s; }

	inline void* alloc(size_t s);
	template<typename T> inline T* alloc(size_t n);
	inline void* realloc(size_t s);
	template<typename T> T* realloc(size_t n);
	inline void free();
	inline void copy(void* src,size_t s);
	template<typename T,std::size_t N> inline void copy(const std::array<T,N>& src);
	template<typename T> inline void copy(const std::vector<T>& src);

};


}


// inline and template functions
namespace ri {

inline Buffer Buffer::createByAlloc(size_t s)  { return createByAlloc<uint8_t>(s); }
template<typename T> static inline Buffer Buffer::createByAlloc(size_t n)  { Buffer b(new T[n],sizeof(T)*n); return b; }
inline Buffer Buffer::createByCopy(const void* src,size_t n)  { Buffer b(new uint8_t[n],n); memcpy(b.data.get(),src,n); return b; }
template<typename T,std::size_t N> static inline Buffer Buffer::createByCopy(const std::array<T,N>& src)  { return createByCopy(src.data(),sizeof(T)*N); }
template<typename T> static inline Buffer Buffer::createByCopy(const std::vector<T>& src)  { return createByCopy(src.data(),sizeof(T)*src.size()); }
inline void* Buffer::alloc(size_t s)  { return alloc<uint8_t>(s); }
template<typename T> inline T* Buffer::alloc(size_t n)  { data.reset(new T[n]); size=sizeof(T)*n; return reinterpret_cast<T*>(data.get()); }
inline void* Buffer::realloc(size_t s)  { return realloc<uint8_t>(s); }
template<typename T> T* Buffer::realloc(size_t n)  { T* a=new T[n]; size_t s=sizeof(T)*n; memcpy(a,data.get(),std::min(s,size)); set(a,s); return reinterpret_cast<T*>(data.get()); }
inline void Buffer::free()  { data.reset(nullptr); size=0; }
inline void Buffer::copy(void* src,size_t s)  { data.reset(new uint8_t[s]); size=s; memcpy(data.get(),src,size); }
template<typename T,std::size_t N> inline void Buffer::copy(const std::array<T,N>& src)  { copy(src.data(),sizeof(T)*N); }
template<typename T> inline void Buffer::copy(const std::vector<T>& src)  { copy(src.data(),sizeof(T)*src.size()); }

}
