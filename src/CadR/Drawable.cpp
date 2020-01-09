#include <CadR/Drawable.h>

using namespace CadR;

static_assert(sizeof(Drawable)==64,
              "Drawable size is expected to be 64 bytes, to fit nicely to CPU caches, etc..\n"
              "Please, consider to tune Drawable::drawCommandList preallocated storage size,\n"
			  "e.g. second template parameter.");



DrawCommandList::DrawCommandList(DrawCommandList&& other) noexcept
	: _capacity(other._capacity)
	, _size(other._size)
{
	if(other._capacity==builtInCapacity) {
		auto& m=renderer()->drawCommandAllocationManager();
		for(uint32_t i=0; i<_size; i++)
			new(_internalStorage[i]) DrawCommand(std::move(other._internalStorage[i]),m);
		other.resize(0);
	} else {
		_externalStorage=other._externalStorage;
		other._capacity=builtInCapacity;
		other._size=0;
	}
}


DrawCommandList& DrawCommandList::operator=(DrawCommandList&& rhs) noexcept
{
	// release previous content
	clear();

	if(rhs._capacity==builtInCapacity) {

		// release external storage
		if(_capacity>builtInCapacity)
			setCapacity(builtInCapacity);

		// move-construct new elements
		auto& m=renderer()->drawCommandAllocationManager();
		for(uint32_t i=0; i<rhs._size; i++)
			new(_internalStorage[i]) DrawCommand(std::move(rhs._internalStorage[i]),m);
		rhs.resize(0);

	} else {
		_capacity=rhs._capacity;
		_externalStorage=rhs._externalStorage;
		rhs._capacity=builtInCapacity;
		rhs._size=0;
	}
	_size=rhs._size;

	return *this;
}


void DrawCommandList::setCapacity(uint32_t newCapacity)
{
	assert(newCapacity>=size() && "Can not set capacity lower than size.");

	// force minimum capacity to builtInCapacity
	if(newCapacity<builtInCapacity)
		newCapacity=builtInCapacity;

	// return on no change
	if(newCapacity==_capacity)
		return;

	// get old and new storage
	DrawCommand* oldStorage=(_capacity==builtInCapacity)?_internalStorage.data():_externalStorage->data();
	DrawCommand* newStorage=(newCapacity==builtInCapacity)
		? _internalStorage.data()
		: reinterpret_cast<DrawCommand*>(new char[newCapacity*sizeof(DrawCommand)]);

	// perform move operation
	try {
		Renderer* r=renderer();
		for(uint32_t i=0; i<_size; i++)
			new(newStorage[i]) DrawCommand(std::move(oldStorage[i]),r);
		for(uint32_t i=0; i<_size; i++)
			oldStorage[i].~DrawCommand();
	} catch(...) {
		if(newCapacity!=builtInCapacity)
			delete[] reinterpret_cast<char*>(newStorage);
		throw;
	}
	if(_capacity!=builtInCapacity)
		delete[] reinterpret_cast<char*>(oldStorage);
	_capacity=newCapacity;
	_externalStoragePointer=reinterpret_cast<char*>(newStorage);
}


void DrawCommandList::resize(uint32_t newSize)
{
	// call constructors or destructors
	auto& m=renderer()->drawCommandAllocationManager();
	if(newSize>=_size) {

		// secure space
		secureSpace(newSize);

		// construct new elements
		for(uint32_t i=_size; i<newSize; i++)
			new(operator[](i)) DrawCommand(m);

	} else

		// destruct elements
		for(uint32_t i=newSize; i<_size; i++) {
			DrawCommand& d=operator[](i);
			d.free(m);
			d.~DrawCommand();
		}

	stateSet()->incrementNumDrawCommands(ptrdiff_t(newSize)-_size);
	_size=newSize;
}


uint32_t DrawCommandList::alloc_back(uint32_t num)
{
	// secure space
	uint32_t first=_size;
	uint32_t end=_size+num;
	secureSpace(end);
	_size=end;
	stateSet()->incrementNumDrawCommands(num);

	// create new DrawCommands
	auto& m=renderer()->drawCommandAllocationManager();
	for(uint32_t i=first; i<end; i++)
		new(operator[](i)) DrawCommand(m);
	return first;
}


void DrawCommandList::alloc_insert(uint32_t index,uint32_t num)
{
	assert(index<=_size && "Parameter index out of bounds.");

	// secure space
	uint32_t newSize=_size+num;
	secureSpace(newSize);

	// create new DrawCommands by move constructor
	uint32_t uninitializedSplit=index+num;
	auto& m=renderer()->drawCommandAllocationManager();
	for(uint32_t i=std::max(_size,uninitializedSplit); i<newSize; i++)
		new(operator[](i)) DrawCommand(std::move(operator[](i-num)),m);

	// alloc DrawCommands
	for(uint32_t i=_size; i<uninitializedSplit; i++)
		new(operator[](i)) DrawCommand(m);

	// move remaining DrawCommands
	for(uint32_t i=_size-num-1; i>=index; i--)
		operator[](i+num).assign(std::move(operator[](i)),m);

	// alloc DrawCommands
	for(uint32_t i=index,e=index+num; i<e; i++)
		operator[](i).alloc(m);

	stateSet()->incrementNumDrawCommands(num);
	_size=newSize;
}


void DrawCommandList::insert(uint32_t index,DrawCommand&& dc)
{
	assert(index<=_size && "Parameter index out of bounds.");

	// secure space
	secureSpace(_size+1);

	// place it at the end of list
	auto& m=renderer()->drawCommandAllocationManager();
	if(index>=_size)
		new(operator[](index)) DrawCommand(std::move(dc),m);

	// insert item
	else {

		// move constructor for the last item
		new(operator[](_size)) DrawCommand(std::move(operator[](_size-1)),m);

		// move remaining DrawCommands
		for(uint32_t i=_size-2; i>=index; i--)
			operator[](i+1).assign(std::move(operator[](i)),m);

		// alloc item
		operator[](index).alloc(m);
	}

	stateSet()->incrementNumDrawCommands(1);
	_size++;
}


void DrawCommandList::move(uint32_t src,uint32_t dst)
{
	assert(src<_size && "Parameter src (source index) out of bounds.");
	assert(dst<=_size && "Parameter dst (destination index) out of bounds.");

	auto& m=renderer()->drawCommandAllocationManager();
	if(dst<_size)

		// move DrawCommand
		operator[](dst).assign(std::move(operator[](src)),m);

	else {

		// secure space
		_size++;
		stateSet()->incrementNumDrawCommands(1);
		secureSpace(_size);

		// create new DrawCommands by move constructor
		new(operator[](dst)) DrawCommand(std::move(operator[](src)),m);
	}
}


void DrawCommandList::move(uint32_t src,uint32_t dst,uint32_t num)
{
	assert(src+num<=_size && "Parameter src (source index) out of bounds.");
	assert(dst<=_size && "Parameter dst (destination index) out of bounds.");

	// do we need extra space?
	auto& m=renderer()->drawCommandAllocationManager();
	uint32_t newSize=dst+num;
	if(newSize>_size)
	{
		// create new DrawCommands by move constructor
		secureSpace(newSize);
		uint32_t shift=dst-src;
		for(uint32_t i=_size; i<newSize; i++)
			new(operator[](i)) DrawCommand(std::move(operator[](i-shift)),m);

		stateSet()->incrementNumDrawCommands(newSize-_size);
		_size=newSize;
	}

	// move remaining DrawCommands
	if(dst>src) {
		uint32_t shift=dst-src;
		for(uint32_t i=std::min(dst+num,_size)-1, e=src+shift; i>=e; i--)
			operator[](i).assign(std::move(operator[](i-shift)),m);
	}
	else
		if(src>dst) {
			uint32_t shift=src-dst;
			for(uint32_t i=src, e=src+num; i>=e; i++)
				operator[](i-shift).assign(std::move(operator[](i)),m);
		}
}


void DrawCommandList::swap(uint32_t i1,uint32_t i2)
{
	ItemAllocationManager& m=renderer()->drawCommandAllocationManager();
	m.swap(m[i1],m[i2]);
}


void DrawCommandList::swap(uint32_t i1,uint32_t i2,uint32_t num)
{
	assert(i1+num<_size && i2+num<_size && "Index out of range.");

	ItemAllocationManager& m=renderer()->drawCommandAllocationManager();
	for(uint32_t i=0; i<num; i++)
		m.swap(m[i1+i],m[i2+i]);
}


void DrawCommandList::free(uint32_t index,uint32_t num)
{
	assert(index+num<_size && "Index out of bounds.");

	// free DrawCommands
	auto& m=renderer()->drawCommandAllocationManager();
	for(uint32_t i=index,e=index+num; i<e; i++)
		operator[](i).free(m);
}


void DrawCommandList::pop_back()
{
	assert(_size>0 && "pop_back() called on empty container.");

	if(_size==0)
		return;
	_size--;
	stateSet()->decrementNumDrawCommands(1);
	DrawCommand& d=operator[](_size);
	d.free(renderer());
	d.~DrawCommand();
}


void DrawCommandList::pop_back(uint32_t num)
{
	assert(_size>=num && "Not enough elements in the container to pop_back().");

	auto& m=renderer()->drawCommandAllocationManager();
	uint32_t start=(_size>=num)?_size-num:0;
	for(uint32_t i=start; i<_size; i++) {
		DrawCommand& d=operator[](i);
		d.free(m);
		d.~DrawCommand();
	}
	stateSet()->decrementNumDrawCommands(_size-start);
	_size=start;
}
