#pragma once

#include <memory>
#include <vector>

namespace CadR {


struct BufferData {

	std::unique_ptr<uint8_t[]> data;
	size_t size;

	BufferData();
	BufferData(size_t s);
	template<typename T> BufferData(size_t n);
	BufferData(void* p,size_t s);
	BufferData(BufferData&&) = default;
	BufferData& operator=(BufferData&&) = default;
	BufferData(BufferData&) = delete;
	BufferData& operator=(BufferData&) = delete;

	static BufferData createByAlloc(size_t s);
	template<typename T> static BufferData createByAlloc(size_t n);
	static BufferData createByCopy(const void* src,size_t n);
	template<typename T,std::size_t N> static BufferData createByCopy(const std::array<T,N>& src);
	template<typename T> static BufferData createByCopy(const std::vector<T>& src);

	void* get();
	template<typename T> T* get();
	void set(void* p,size_t s);

	void* alloc(size_t s);
	template<typename T> T* alloc(size_t n);
	void* realloc(size_t s);
	template<typename T> T* realloc(size_t n);
	void free();
	void copy(void* src,size_t s);
	template<typename T,std::size_t N> void copy(const std::array<T,N>& src);
	template<typename T> void copy(const std::vector<T>& src);

};


}


// inline and template functions
#include <cstring>
namespace CadR {

inline BufferData::BufferData() : size(0)  {}
inline BufferData::BufferData(size_t s) : data(new uint8_t[s]), size(s)  {}
template<typename T> inline BufferData::BufferData(size_t n) : data(new T[n]), size(sizeof(T)*n)  {}
inline BufferData::BufferData(void* p,size_t s) : data(reinterpret_cast<uint8_t*>(p)), size(s) {}
inline BufferData BufferData::createByAlloc(size_t s)  { return createByAlloc<uint8_t>(s); }
template<typename T> inline BufferData BufferData::createByAlloc(size_t n)  { BufferData b(new T[n],sizeof(T)*n); return b; }
inline BufferData BufferData::createByCopy(const void* src,size_t n)  { BufferData b(new uint8_t[n],n); memcpy(b.data.get(),src,n); return b; }
template<typename T,std::size_t N> inline BufferData BufferData::createByCopy(const std::array<T,N>& src)  { return createByCopy(src.data(),sizeof(T)*N); }
template<typename T> inline BufferData BufferData::createByCopy(const std::vector<T>& src)  { return createByCopy(src.data(),sizeof(T)*src.size()); }
inline void* BufferData::get()  { return data.get(); }
template<typename T> inline T* BufferData::get()  { return reinterpret_cast<T*>(data.get()); }
inline void BufferData::set(void* p,size_t s)  { data.reset(static_cast<uint8_t*>(p)); size=s; }
inline void* BufferData::alloc(size_t s)  { return alloc<uint8_t>(s); }
template<typename T> inline T* BufferData::alloc(size_t n)  { data.reset(new T[n]); size=sizeof(T)*n; return reinterpret_cast<T*>(data.get()); }
inline void* BufferData::realloc(size_t s)  { return realloc<uint8_t>(s); }
template<typename T> T* BufferData::realloc(size_t n)  { T* a=new T[n]; size_t s=sizeof(T)*n; memcpy(a,data.get(),std::min(s,size)); set(a,s); return reinterpret_cast<T*>(data.get()); }
inline void BufferData::free()  { data.reset(nullptr); size=0; }
inline void BufferData::copy(void* src,size_t s)  { data.reset(new uint8_t[s]); size=s; memcpy(data.get(),src,size); }
template<typename T,std::size_t N> inline void BufferData::copy(const std::array<T,N>& src)  { copy(src.data(),sizeof(T)*N); }
template<typename T> inline void BufferData::copy(const std::vector<T>& src)  { copy(src.data(),sizeof(T)*src.size()); }

}
