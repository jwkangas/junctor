/*
 *  General stuff
 *  
 *  Copyright 2015 Teppo Niinimäki <teppo.niinimaki(at)helsinki.fi>
 *  
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef COMMON_HPP
#define COMMON_HPP

#include <cassert>


/**
 * General exception.
 */
class Exception {
private:
	boost::format msg_;
	
public:
	Exception(const std::string& msg) : msg_(msg) {
	}
	
	Exception(const boost::format& msg) : msg_(msg) {
	}
	
	template <typename T>
	Exception operator%(const T& x) {
		return Exception(msg_ % x);
	}
	
	const char* what() {
		return str(msg_).c_str();
	}
};



struct DummySet {
	void clear() {}
	void insert(int x) {}
	void remove(int x) {}
	void insertLargest(int x) {}
	void removeLargest(int x) {}
	void insertSmallest(int x) {}
	void removeSmallest(int x) {}
};


template <class Set1, class Set2>
struct SetPair : public std::pair<Set1, Set2> {
	using std::pair<Set1, Set2>::first;
	using std::pair<Set1, Set2>::second;
	
	constexpr SetPair() :
		std::pair<Set1, Set2>()
	{}

	SetPair(const Set1& set1, const Set2& set2) :
		std::pair<Set1, Set2>(set1, set2)
	{}

	SetPair(Set1&& set1, Set2&& set2) :
		std::pair<Set1, Set2>(set1, set2)
	{}
	
	SetPair(const SetPair& other) :
		std::pair<Set1, Set2>(other)
	{}

	SetPair(SetPair&& other) :
		std::pair<Set1, Set2>(other)
	{}
	
	void clear() {
		first.clear;
		second.clear;
	}
	
	void insert(int x) {
		first.insert(x);
		second.insert(x);
	}
	
	void remove(int x) {
		first.remove(x);
		second.remove(x);
	}
	
	void insertLargest(int x) {
		first.insertLargest(x);
		second.insertLargest(x);
	}
	
	void removeLargest(int x) {
		first.removeLargest(x);
		second.removeLargest(x);
	}

	void insertSmallest(int x) {
		first.insertSmallest(x);
		second.insertSmallest(x);
	}
	
	void removeSmallest(int x) {
		first.removeSmallest(x);
		second.removeSmallest(x);
	}
};




/*template <typename Range>
struct ConstReverse {
private:
	const Range& r_;
public:
	ConstReverse(const Range& r) :
		r_(r)
	{
	}
	
	typename Range::ConstReverseIterator begin() const {
		return r_.rbegin();
	}
		
	typename Range::ConstReverseIterator end() const {
		return r_.rend();
	}
};

template <typename Range>
ConstReverse<Range> reverse(const Range& r) {
	return ConstReverse<Range>(r);
}*/


template <typename Range>
struct Reverse {
private:
	Range& r_;
public:
	Reverse(Range& r) :
		r_(r)
	{}
	
	decltype(r_.rbegin()) begin() {
		return r_.rbegin();
	}
		
	decltype(r_.rbegin()) end() {
		return r_.rend();
	}
};

template <typename Range>
Reverse<Range> reverse(Range& r) {
	return Reverse<Range>(r);
}


#endif


