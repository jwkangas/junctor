/*
 *  Subsets stored as a an array of sorted elements
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

#include <iostream>

#include <cassert>

#ifndef SORTEDSUBSET_HPP
#define SORTEDSUBSET_HPP


/**
 * SortedSubsetRange
 */
struct SortedSubsetRange {
protected:
	int* begin_;
	int* end_;
	
	SortedSubsetRange& operator=(const SortedSubsetRange&) = delete; // disable assignment

	SortedSubsetRange() {
	}
public:
	/*SortedSubsetRange(size_t size, int* elements) {
		size_ = size;
		elements_ = elements;
	}*/
	
	/*SortedSubsetRange(const SortedSubsetRange& other) {
		size_ = other.size_;
		elements_ = other.elements_;
	}*/
	
	bool operator==(const SortedSubsetRange& other) const {
		if (size() != other.size())
			return false;
		for (int i = 0; i < size(); ++i)
			if (begin_[i] != other.begin_[i])
				return false;
		return true;
	}
	
	bool operator!=(const SortedSubsetRange& other) const {
		return !operator==(other);
	}

	size_t size() const {
		return end_ - begin_;
	}
	
	bool empty() const {
		return begin_ == end_;
	}
	
	int operator[] (int i) const {
		assert(0 <= i && i < size());
		return *(begin_ + i);
	}
	
	int getSmallest() const {
		assert(!empty());
		return *begin_;
	}
	
	int getLargest() const {
		assert(!empty());
		return *(end_ - 1);
	}

	bool contains(int x) const {
		for (auto elem : *this)
			if (x == elem)
				return true;
		return false;
	}
	
	struct ConstIterator : std::iterator<std::bidirectional_iterator_tag, int> {
	private:
		int* elem_;

		ConstIterator(int* elem) {
			elem_ = elem;
		}
		
		friend struct SortedSubsetRange;
		
	public:
		ConstIterator& operator++() {
			++elem_;
			return *this;
		}
		
		ConstIterator& operator--() {
			--elem_;
			return *this;
		}
		
		bool operator!=(const ConstIterator& other) {
			return elem_ != other.elem_;
		}
		
		int operator*() {
			return *elem_;
		}
	};

	ConstIterator begin() const {
		return ConstIterator(begin_);
	}
		
	ConstIterator end() const {
		return ConstIterator(end_);
	}

	struct ConstReverseIterator : std::iterator<std::forward_iterator_tag, int> {
	private:
		int* nextElem_;
		
		ConstReverseIterator(int* nextElem) {
			nextElem_ = nextElem;
		}

		friend struct SortedSubsetRange;
		
	public:
		ConstReverseIterator& operator++() {
			--nextElem_;
			return *this;
		}
		
		bool operator!=(const ConstReverseIterator& other) {
			return nextElem_ != other.nextElem_;
		}
		
		int operator*() {
			return *(nextElem_ - 1);
		}
	};
	
	ConstReverseIterator rbegin() const {
		return ConstReverseIterator(end_);
	}
	
	ConstReverseIterator rend() const {
		return ConstReverseIterator(begin_);
	}
	
	//friend std::ostream& operator<<(std::ostream& os, const SortedSubsetRange& ss);
};


bool isSubsetOf(const SortedSubsetRange& a, const SortedSubsetRange& b) {
	int i = 0;
	int j = 0;
	while (i < a.size()) {
		if (j >= b.size()) 
			return false;
		while (b[j] < a[i]) {
			++j;
			if (j >= b.size()) 
				return false;
		}
		if (a[i] != b[j])
			return false;
		++i;
		++j;
	}
	return true;
}


std::ostream& operator<<(std::ostream& os, const SortedSubsetRange& ss) {
	if (ss.size() == 0)
		os << "∅";
	else {
		os << "{" << ss[0];
		for (int i = 1; i < ss.size(); ++i)
			os << "," << ss[i];
		os << "}";
	}
	return os;
}




/**
 * SortedStackSubset
 */
struct SortedStackSubset : public SortedSubsetRange {
private:
	int* maxEnd_;
	
	SortedStackSubset& operator=(const SortedStackSubset&) = delete; // disable assignment
public:
	SortedStackSubset() {
		begin_ = nullptr;
		end_ = nullptr;
		maxEnd_ = nullptr;
	}
	
	SortedStackSubset(int maxSize) {
		begin_ = new int[maxSize];
		end_ = begin_;
		maxEnd_ = begin_ + maxSize;
	}
	
	SortedStackSubset(const SortedStackSubset& other) :
		SortedSubsetRange(other)
	{
		size_t maxSize = other.maxEnd_ - other.begin_;
		begin_ = new int[maxSize];
		end_ = begin_;
		maxEnd_ = begin_ + maxSize;
		for (auto elem : other)
			insertLargest(elem);
	}
	
	SortedStackSubset(SortedStackSubset&& other) {
		begin_ = other.begin_;
		end_ = other.end_;
		maxEnd_ = other.maxEnd_;
		other.begin_ = nullptr;
		other.end_ = nullptr;
		other.maxEnd_ = nullptr;
	}

	SortedStackSubset(int maxSize, const SortedSubsetRange& other) :
		SortedSubsetRange(other)
	{
		assert(maxSize >= other.size());
		begin_ = new int[maxSize];
		end_ = begin_;
		maxEnd_ = begin_ + maxSize;
		for (auto elem : other)
			(*this).insertLargest(elem);
	}
	
	SortedStackSubset(int maxSize, const std::vector<int>& elems) {
		assert(maxSize >= elems.size());
		begin_ = new int[maxSize];
		end_ = begin_;
		maxEnd_ = begin_ + maxSize;
		for (auto elem : elems)
			*(end_++) = elem;
		std::sort(begin_, end_);
	}
	
	~SortedStackSubset() {
		delete[] begin_;
	}
	
	void clear() {
		end_ = begin_;
	}
	
	// TODO: ei pitäisi olla sallittu
	/*int& operator[] (int i) {
		assert(0 <= i && i < size_);
		return elements_[i];
	}*/

	void insertLargest(int elem) {
		assert(end_ < maxEnd_);
		assert(empty() || getLargest() < elem);
		*(end_++) = elem;
	}

	void removeLargest(int elem) {
		assert(!empty());
		--end_;
		assert(*end_ == elem);
	}
	
	void insert(int elem) {
		assert(end_ < maxEnd_);
		int* iter = end_;
		++end_;
		while(iter > begin_ && *(iter - 1) > elem) {
			*iter = *(iter - 1);
			--iter;
		}
		assert(iter == begin_ || *(iter - 1) < elem);
		*iter = elem;
	}
	
	void remove(int elem) {
		assert(!empty());
		--end_;
		int* iter = end_;
		int curr = *iter;
		--iter;
		while(iter >= begin_ && curr > elem) {
			int tmp = *iter;
			*iter = curr;
			curr = tmp;
			--iter;
		}
		assert(curr == elem);
	}

	static SortedStackSubset fullSet(int n) {
		SortedStackSubset s(n);
		for (int i = 0; i < n; ++i)
			s.insertLargest(i);
		return s;
	}
	
};/**/



/**
 * SortedDownStackSubset
 */
struct SortedDownStackSubset : public SortedSubsetRange {
private:
	int* minBegin_;
	
	SortedDownStackSubset& operator=(const SortedDownStackSubset&) = delete; // disable assignment
public:
	SortedDownStackSubset() {
		begin_ = nullptr;
		end_ = nullptr;
		minBegin_ = nullptr;
	}
	
	SortedDownStackSubset(int maxSize) {
		minBegin_ = new int[maxSize];
		end_ = minBegin_ + maxSize;
		begin_ = end_;
	}

	SortedDownStackSubset(const SortedDownStackSubset& other) {
		size_t maxSize = other.end_ - other.minBegin_;
		minBegin_ = new int[maxSize];
		end_ = minBegin_ + maxSize;
		begin_ = end_;
		for (auto elem : reverse(other))
			insertSmallest(elem);
	}
	
	SortedDownStackSubset(SortedDownStackSubset&& other) {
		begin_ = other.begin_;
		end_ = other.end_;
		minBegin_ = other.minBegin_;
		other.begin_ = nullptr;
		other.end_ = nullptr;
		other.minBegin_ = nullptr;
	}

	SortedDownStackSubset(int maxSize, const SortedSubsetRange& other) {
		assert(maxSize >= other.size());
		minBegin_ = new int[maxSize];
		end_ = minBegin_ + maxSize;
		begin_ = end_;
		for (auto elem : reverse(other))
			(*this).insertSmallest(elem);
	}
	
	SortedDownStackSubset(int maxSize, const std::vector<int>& elems) {
		assert(maxSize >= elems.size());
		minBegin_ = new int[maxSize];
		end_ = minBegin_ + maxSize;
		begin_ = end_;
		for (auto elem : elems)
			*(--begin_) = elem;
		std::sort(begin_, end_);
	}
	
	~SortedDownStackSubset() {
		delete[] minBegin_;
	}
	
	void clear() {
		begin_ = end_;
	}
	

	void insertSmallest(int elem) {
		assert(begin_ > minBegin_);
		assert(empty() || getSmallest() > elem);
		*(--begin_) = elem;
	}

	void removeSmallest(int elem) {
		assert(!empty());
		assert(*begin_ == elem);
		++begin_;
	}
	
	/*void insert(int elem) {
		assert(end_ < maxEnd_);
		int* iter = end_;
		++end_;
		while(iter > begin_ && *(iter - 1) > elem) {
			*iter = *(iter - 1);
			--iter;
		}
		assert(iter == begin_ || *(iter - 1) < elem);
		*iter = elem;
	}
	
	void remove(int elem) {
		assert(!empty());
		--end_;
		int* iter = end_;
		int curr = *iter;
		--iter;
		while(iter >= begin_ && curr > elem) {
			int tmp = *iter;
			*iter = curr;
			curr = tmp;
			--iter;
		}
		assert(curr == elem);
	}*/

};/**/



/**
 * ConstSortedSubset
 */
struct ConstSortedSubset : public SortedSubsetRange {
private:
	ConstSortedSubset& operator=(const ConstSortedSubset&) = delete; // disable assignment

public:
	ConstSortedSubset() {
		begin_ = nullptr;
		end_ = nullptr;
	}
	
	ConstSortedSubset(const SortedSubsetRange& other) {
		size_t size = other.size();
		begin_ = new int[size];
		end_ = begin_ + size;
		for (int i = 0; i < size; ++i)
			*(begin_ + i) = other[i];
	}

	// should be unnecessary! (the above should suffice)
	ConstSortedSubset(const ConstSortedSubset& other) :
		ConstSortedSubset((const SortedSubsetRange&) other)
		//SortedSubsetRange(other.size(), new int[other.size()])
	{
		//for (int i = 0; i < size_; ++i)
		//	elements_[i] = other[i];
	}

	// should be unnecessary! (the above should suffice)
	/*ConstSortedSubset(const SortedStackSubset& other) {
		size_ = other.size();
		elements_ = new int[size_];
		for (int i = 0; i < size_; ++i)
			elements_[i] = other[i];
	}*/
	
	ConstSortedSubset(ConstSortedSubset&& other) {
		begin_ = other.begin_;
		end_ = other.end_;
		other.begin_ = nullptr;
		other.end_ = nullptr;
	}

	ConstSortedSubset(const std::vector<int>& elems) {
		size_t size = elems.size();
		begin_ = new int[size];
		end_ = begin_ + size;
		for (int i = 0; i < size; ++i)
			*(begin_ + i) = elems[i];
		std::sort(begin_, end_);
	}
	
	~ConstSortedSubset() {
		delete[] begin_;
	}

	static ConstSortedSubset fullSet(int n) {
		ConstSortedSubset s;
		s.begin_ = new int[n];
		s.end_ = s.begin_;
		for (int i = 0; i < n; ++i)
			*(s.end_++) = i;
		return s;
	}
};



#endif

