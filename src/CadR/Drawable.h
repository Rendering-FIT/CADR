#pragma once

#include <CadR/Export.h>
#include <CadR/DrawCommand.h>
#include <boost/intrusive/list.hpp>

namespace CadR {

class MatrixList;
class Mesh;
class Renderer;
class StateSet;


struct DrawCommandList {
public:
	static constexpr const uint32_t builtInCapacity = 6;
protected:

	uint32_t _capacity;
	uint32_t _size;
	union {
		std::array<DrawCommand,builtInCapacity> _internalStorage;
		std::array<DrawCommand,0>* _externalStorage;
		char* _externalStoragePointer;
	};

	Renderer* renderer() const;
	void secureSpace(uint32_t newSize);

public:

	using iterator =  std::array<DrawCommand,0>::iterator;
	using const_iterator = std::array<DrawCommand,0>::const_iterator;

	DrawCommandList();
	DrawCommandList(uint32_t capacity);
	DrawCommandList(DrawCommandList&& other) noexcept;
	~DrawCommandList();
	DrawCommandList& operator=(DrawCommandList&& rhs) noexcept;

	void reserve(uint32_t capacity);
	void setCapacity(uint32_t newCapacity);
	void resize(uint32_t newSize);
	uint32_t capacity() const;
	uint32_t size() const;
	void shrink_to_fit();

	uint32_t alloc_back(uint32_t num=1);
	void alloc_insert(uint32_t index,uint32_t num=1);
	uint32_t emplace_back(DrawCommand&& dc);
	void insert(uint32_t index,DrawCommand&& dc);

	void set(uint32_t index,DrawCommand&& dc);
	void move(uint32_t src,uint32_t dst);
	void move(uint32_t src,uint32_t dst,uint32_t num);
	void swap(uint32_t i1,uint32_t i2);
	void swap(uint32_t i1,uint32_t i2,uint32_t num);

	void free(uint32_t index,uint32_t num=1);
	void pop_back();
	void pop_back(uint32_t num);
	void clear();

	iterator begin();
	const_iterator cbegin() const;
	iterator end();
	const_iterator cend() const;

	DrawCommand& operator[](uint32_t index);
	const DrawCommand& operator[](uint32_t index) const;
	DrawCommand* data();
	const DrawCommand* data() const;

};


class CADR_EXPORT Drawable final {
public:

	boost::intrusive::list_member_hook<> drawableListHook;
	MatrixList* matrixList;
	StateSet* stateSet;
	DrawCommandList drawCommandList;

	Drawable(Mesh* mesh,MatrixList* matrixList,StateSet* stateSet,uint32_t numDrawCommands);
	Drawable(Drawable&& other) = default;
	Drawable& operator=(Drawable&& rhs) = default;

	Renderer* renderer() const;

	void setNumDrawCommands(uint32_t num);
	uint32_t numDrawCommands() const;
	void uploadDrawCommand(uint32_t index,const DrawCommandGpuData& drawCommandData);
	StagingBuffer createDrawCommandStagingBuffer(uint32_t index);

	DrawCommand*const& drawCommandAllocation(uint32_t index) const;  ///< Returns drawCommand allocation for drawCommand on particular index in drawCommandList.
	DrawCommand*& drawCommandAllocation(uint32_t index);   ///< Returns drawCommand allocation for drawCommand on particular index in drawCommandList. Modify the returned data only with caution.

	Drawable() = delete;
	Drawable(const Drawable&) = delete;
	Drawable& operator=(const Drawable&) = delete;

};


typedef boost::intrusive::list<
	Drawable,
	boost::intrusive::member_hook<
		Drawable,
		boost::intrusive::list_member_hook<>,
		&Drawable::drawableListHook>
> DrawableList;


}


// inline methods
#include <CadR/Renderer.h>
#include <CadR/StateSet.h>
namespace CadR {

inline Drawable::Drawable(Mesh*,MatrixList* matrixList_,StateSet* stateSet_,uint32_t numDrawCommands)
	: matrixList(matrixList_), stateSet(stateSet_), drawCommandList(numDrawCommands)  {}

inline Renderer* Drawable::renderer() const  { return stateSet->renderer(); }
inline void Drawable::setNumDrawCommands(uint32_t num)  { drawCommandList.resize(num); }
inline uint32_t Drawable::numDrawCommands() const  { return drawCommandList.size(); }
inline void Drawable::uploadDrawCommand(uint32_t index,const DrawCommandGpuData& drawCommandData)  { renderer()->uploadDrawCommand(drawCommandList[index],drawCommandData); }
inline StagingBuffer Drawable::createDrawCommandStagingBuffer(uint32_t index)  { return renderer()->createDrawCommandStagingBuffer(drawCommandList[index]); }
inline DrawCommand*const& Drawable::drawCommandAllocation(uint32_t index) const  { return reinterpret_cast<DrawCommand*const&>(renderer()->drawCommandAllocation(drawCommandList[index].index())); }
inline DrawCommand*& Drawable::drawCommandAllocation(uint32_t index)  { return reinterpret_cast<DrawCommand*&>(renderer()->drawCommandAllocation(drawCommandList[index].index())); }

inline Renderer* DrawCommandList::renderer() const  { return reinterpret_cast<const Drawable*>(reinterpret_cast<const char*>(this)-size_t(&static_cast<Drawable*>(nullptr)->drawCommandList))->renderer(); }
inline DrawCommandList::DrawCommandList() : _capacity(builtInCapacity), _size(0)  {}
inline DrawCommandList::DrawCommandList(uint32_t capacity) : DrawCommandList()  { reserve(capacity); }
inline DrawCommandList::~DrawCommandList()  { clear(); if(_capacity!=builtInCapacity) delete[] _externalStoragePointer; }

inline void DrawCommandList::reserve(uint32_t newCapacity)  { if(newCapacity>_capacity) setCapacity(newCapacity); }
inline uint32_t DrawCommandList::capacity() const  { return _capacity; }
inline uint32_t DrawCommandList::size() const  { return _size; }
inline void DrawCommandList::shrink_to_fit()  { setCapacity(size()); }
inline void DrawCommandList::secureSpace(uint32_t newSize)  { if(newSize<_capacity) return; uint32_t c=_capacity*2; if(c<newSize) c=newSize; setCapacity(c); }

inline uint32_t DrawCommandList::emplace_back(DrawCommand&& dc)  { secureSpace(_size+1); new(operator[](_size)) DrawCommand(std::move(dc),renderer()); return _size++; }
inline void DrawCommandList::set(uint32_t index,DrawCommand&& dc)  { assert(index<_size && "Parameter index out of bounds."); operator[](index).assign(std::move(dc),renderer()); }

inline void DrawCommandList::clear()  { resize(0); }

inline DrawCommandList::iterator DrawCommandList::begin()  { if(_capacity==builtInCapacity) return reinterpret_cast<std::array<DrawCommand,0>*>(&_internalStorage)->begin(); else return _externalStorage->begin(); }
inline DrawCommandList::const_iterator DrawCommandList::cbegin() const  { if(_capacity==builtInCapacity) return reinterpret_cast<const std::array<DrawCommand,0>*>(&_internalStorage)->begin(); else return _externalStorage->begin(); }
inline DrawCommandList::iterator DrawCommandList::end()  { if(_capacity==builtInCapacity) return reinterpret_cast<std::array<DrawCommand,0>*>(&_internalStorage)->end(); else return _externalStorage->end(); }
inline DrawCommandList::const_iterator DrawCommandList::cend() const  { if(_capacity==builtInCapacity) return reinterpret_cast<const std::array<DrawCommand,0>*>(&_internalStorage)->end(); else return _externalStorage->end(); }
inline DrawCommand& DrawCommandList::operator[](uint32_t index)  { if(_capacity==builtInCapacity) return _internalStorage[index]; else return _externalStorage->operator[](index); }
inline const DrawCommand& DrawCommandList::operator[](uint32_t index) const  { if(_capacity==builtInCapacity) return _internalStorage[index]; else return _externalStorage->operator[](index); }
inline DrawCommand* DrawCommandList::data()  { if(_capacity==builtInCapacity) return _internalStorage.data(); else return _externalStorage->data(); }
inline const DrawCommand* DrawCommandList::data() const  { if(_capacity==builtInCapacity) return _internalStorage.data(); else return _externalStorage->data(); }

}
