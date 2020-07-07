#pragma once

#include <boost/intrusive/list.hpp>

namespace CadR {


template<typename Type>
struct ParentChildRelation {
	boost::intrusive::list_member_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink>
	> childListHook;  ///< Hook to Parent::childList.
	Type* child;
	Type* parent;
	boost::intrusive::list_member_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink>
	> parentListHook;  ///< Hook to Child::parentList.
};


template<typename InternalIteratorType,typename Type,bool ReturnChildOrParent>
class ParentChildIterator {
protected:
	typename InternalIteratorType _it;
public:
	typedef std::bidirectional_iterator_tag iterator_category;

	inline ParentChildIterator(typename InternalIteratorType it) : _it(it)  {}

	inline typename InternalIteratorType internalIterator() const  { return _it; }

	template<bool R=ReturnChildOrParent> inline typename std::enable_if< R,Type >::type& operator*()  const  { return *_it->child;  }
	template<bool R=ReturnChildOrParent> inline typename std::enable_if<!R,Type >::type& operator*()  const  { return *_it->parent; }
	template<bool R=ReturnChildOrParent> inline typename std::enable_if< R,Type*>::type  operator->() const  { return  _it->child;  }
	template<bool R=ReturnChildOrParent> inline typename std::enable_if<!R,Type*>::type  operator->() const  { return  _it->parent; }

	inline ParentChildIterator& operator++()     { _it++; return *this; }
	inline ParentChildIterator  operator++(int)  { ParentChildIterator tmp=*this; _it++; return tmp; }
	inline ParentChildIterator& operator--()     { _it--; return *this; }
	inline ParentChildIterator  operator--(int)  { ParentChildIterator tmp=*this; _it--; return tmp; }

	inline bool operator==(const ParentChildIterator& rhs) const  { return _it == rhs._it; }
	inline bool operator!=(const ParentChildIterator& rhs) const  { return _it != rhs._it; }
};


template<typename Type>
class ChildList {
public:
	typedef boost::intrusive::list<
		ParentChildRelation<Type>,
		boost::intrusive::member_hook<
			ParentChildRelation<Type>,
			boost::intrusive::list_member_hook<
				boost::intrusive::link_mode<boost::intrusive::auto_unlink>>,
			&ParentChildRelation<Type>::childListHook>,
		boost::intrusive::constant_time_size<false>
	> List;
protected:
	List _list;
	uint32_t _childListOffset;
	uint32_t _parentListOffset;
public:

	List& internalList()  { return _list; }
	const List& internalList() const  { return _list; }

	typedef ParentChildIterator<typename List::iterator,Type,true> iterator;
	typedef ParentChildIterator<typename List::const_iterator,Type,true> const_iterator;
	typedef ParentChildIterator<typename List::reverse_iterator,Type,true> reverse_iterator;
	typedef ParentChildIterator<typename List::const_reverse_iterator,Type,true> const_reverse_iterator;

	Type& front()  { return *_list.front().child; }
	const Type& front() const  { return *_list.front().child; }
	Type& back()  { return *_list.back().child; }
	const Type& back() const  { return *_list.back().child; }
	iterator begin()  { return _list.begin(); }
	const_iterator begin() const  { return _list.begin(); }
	const_iterator cbegin() const  { return _list.cbegin(); }
	iterator end()  { return _list.end(); }
	const_iterator end() const  { return _list.end(); }
	const_iterator cend() const  { return _list.cend(); }
	reverse_iterator rbegin()  { return _list.rbegin(); }
	const_reverse_iterator rbegin() const  { return _list.rbegin(); }
	const_reverse_iterator crbegin() const  { return _list.crbegin(); }
	reverse_iterator rend()  { return _list.rend(); }
	const_reverse_iterator rend() const  { return _list.rend(); }
	const_reverse_iterator crend() const  { return _list.crend(); }
	size_t size() const  { return _list.size(); }
	bool empty() const  { return _list.empty(); }

	ChildList(size_t childListOffset,size_t parentListOffset) : _childListOffset(uint32_t(childListOffset)), _parentListOffset(uint32_t(parentListOffset))  {}
	iterator append(Type* child) {
		auto* r=new ParentChildRelation<Type>;
		_list.push_back(*r);
		r->child=child;
		r->parent=reinterpret_cast<Type*>(reinterpret_cast<char*>(this)-_childListOffset);
		reinterpret_cast<ParentList<Type>*>(reinterpret_cast<char*>(child)+_parentListOffset)->internalList().push_back(*r);
		return List::s_iterator_to(*r);
	}
	void remove(iterator it) {
		auto iit=it.internalIterator();
		_list.erase(iit);
		auto* parentList=reinterpret_cast<ParentList<Type>*>(reinterpret_cast<char*>(iit->child)+_parentListOffset);
		parentList->internalList().erase(ParentList<Type>::List::s_iterator_to(*iit));
		delete &*iit;
	}
	void clear()  { while(!empty()) remove(begin()); }
	~ChildList()  { clear(); }

};


template<typename Type>
class ParentList {
public:
	typedef boost::intrusive::list<
		ParentChildRelation<Type>,
		boost::intrusive::member_hook<
			ParentChildRelation<Type>,
			boost::intrusive::list_member_hook<
				boost::intrusive::link_mode<boost::intrusive::auto_unlink>>,
			&ParentChildRelation<Type>::parentListHook>,
		boost::intrusive::constant_time_size<false>
	> List;
protected:
	List _list;
	uint32_t _childListOffset;
	uint32_t _parentListOffset;
public:

	List& internalList()  { return _list; }
	const List& internalList() const  { return _list; }

	typedef ParentChildIterator<typename List::iterator,Type,false> iterator;
	typedef ParentChildIterator<typename List::const_iterator,Type,false> const_iterator;
	typedef ParentChildIterator<typename List::reverse_iterator,Type,false> reverse_iterator;
	typedef ParentChildIterator<typename List::const_reverse_iterator,Type,false> const_reverse_iterator;

	Type& front()  { return *_list.front().child; }
	const Type& front() const  { return *_list.front().child; }
	Type& back()  { return *_list.back().child; }
	const Type& back() const  { return *_list.back().child; }
	iterator begin()  { return _list.begin(); }
	const_iterator begin() const  { return _list.begin(); }
	const_iterator cbegin() const  { return _list.cbegin(); }
	iterator end()  { return _list.end(); }
	const_iterator end() const  { return _list.end(); }
	const_iterator cend() const  { return _list.cend(); }
	reverse_iterator rbegin()  { return _list.rbegin(); }
	const_reverse_iterator rbegin() const  { return _list.rbegin(); }
	const_reverse_iterator crbegin() const  { return _list.crbegin(); }
	reverse_iterator rend()  { return _list.rend(); }
	const_reverse_iterator rend() const  { return _list.rend(); }
	const_reverse_iterator crend() const  { return _list.crend(); }
	size_t size() const  { return _list.size(); }
	bool empty() const  { return _list.empty(); }

	ParentList(size_t parentListOffset,size_t childListOffset) : _childListOffset(uint32_t(childListOffset)), _parentListOffset(uint32_t(parentListOffset))  {}
	iterator append(Type* parent) {
		auto* r=new ParentChildRelation<Type>;
		reinterpret_cast<ChildList<Type>*>(reinterpret_cast<char*>(parent)+_childListOffset)->internalList().push_back(*r);
		r->child=reinterpret_cast<Type*>(reinterpret_cast<char*>(this)-_parentListOffset);
		r->parent=parent;
		_list.push_back(*r);
		return List::s_iterator_to(*r);
	}
	void remove(iterator it) {
		auto iit=it.internalIterator();
		_list.erase(iit);
		auto* childList=reinterpret_cast<ChildList<Type>*>(reinterpret_cast<char*>(iit->parent)+_childListOffset);
		childList->internalList().erase(ChildList<Type>::List::s_iterator_to(*iit));
		delete &*iit;
	}
	void clear()  { while(!empty()) remove(begin()); }
	~ParentList()  { clear(); }

};


}
