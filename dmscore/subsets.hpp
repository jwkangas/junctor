/*
 *  Subset collections and iterators
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

#include "sortedsubset.hpp"

#ifndef SUBSETS_HPP
#define SUBSETS_HPP


unsigned long numBoundedSubsets(unsigned long n, unsigned long k) {
	unsigned long sum = 1;
	unsigned long perLevel = 1;
	for (unsigned long i = 1; i <= k; ++i) {
		perLevel *= (n-i+1);
		perLevel /= i;
		sum += perLevel;
	}
	return sum;
}


/**
 * A collection of subsets of bounded size
 */
struct BoundedSubsets {
private:
	ConstSortedSubset set_;
	int maxSize_;

public:
	BoundedSubsets(const ConstSortedSubset& set, int maxSize) :
		set_(set),
		maxSize_(maxSize)
	{}
	
	unsigned long size() const {
		return numBoundedSubsets(set_.size(), maxSize_);
	}
	
	int maxSubsetSize() const {
		return maxSize_;
	}
	
	SortedSubsetRange groundSet() const {
		return SortedSubsetRange(set_);
	}
	
	template <typename Subset>
	struct BinaryAscRange;
	
	template <typename Subset>
	BinaryAscRange<Subset> binaryAsc(const Subset& subset) const {
		return BinaryAscRange<Subset>(*this, subset);
	}

	template <typename Subset>
	struct LexicalRange;
	
	template <typename Subset>
	LexicalRange<Subset> lexical(const Subset& subset) const {
		return LexicalRange<Subset>(*this, subset);
	}
};

BoundedSubsets boundedSubsets(const ConstSortedSubset& set, int maxSize) {
	return BoundedSubsets(set, maxSize);
}



/**
 * BinaryAscRange
 */
template <typename Subset>
struct BoundedSubsets::BinaryAscRange {
private:
	BoundedSubsets range_;
	Subset subset_;
	
	//friend struct BoundedSubsets;
	
public:
	BinaryAscRange(const BoundedSubsets& range, const Subset& subset) :
		range_(range),
		subset_(subset)
	{}

	struct Iterator {
	private:
		const BoundedSubsets& range_;
		int round_;
		SortedDownStackSubset ssi_;
		Subset subset_;

		Iterator(const BoundedSubsets& range, int round, const SortedSubsetRange& ssi, Subset subset) :
			range_(range), round_(round), ssi_(range.set_.size() + 1, ssi), subset_(subset)
		{
		}
		
		friend struct BinaryAscRange;
	public:
	
		Iterator& operator++() {
			assert(ssi_.size() <= range_.maxSize_);
			if (range_.set_.size() == 0 || range_.maxSize_ == 0) {
				++round_;
				return *this;
			}
			if (ssi_.empty() || (ssi_.size() < range_.maxSize_
					&& ssi_.getSmallest() > 0)) {
				ssi_.insertSmallest(0);
				subset_.insertSmallest(range_.set_[0]);
				return *this;
			}
		
			int next;
			do {
				int prev = ssi_.getSmallest();
				ssi_.removeSmallest(prev);
				subset_.removeSmallest(range_.set_[prev]);
				next = prev + 1;
				if (ssi_.empty()) {
					if (next == range_.set_.size()) {
						++round_;
						return *this;
					} else
						break;
				}
			} while (next == ssi_.getSmallest());
			ssi_.insertSmallest(next);
			subset_.insertSmallest(range_.set_[next]);
			return *this;
		}
	
		bool operator!=(const Iterator& other) {
			return round_ != other.round_ || ssi_ != other.ssi_;
		}
	
		const Subset& operator*() {
			return subset_;
		}
	};

	Iterator begin() {
		return Iterator(range_, 0, ConstSortedSubset(), subset_);
	}

	Iterator end() {
		return Iterator(range_, 1, ConstSortedSubset(), subset_);
	}
};



/**
 * LexicalRange
 */
template <typename Subset>
struct BoundedSubsets::LexicalRange {
private:
	BoundedSubsets range_;
	Subset subset_;
	
	//friend struct BoundedSubsets;
	
public:
	LexicalRange(const BoundedSubsets& range, const Subset& subset) :
		range_(range),
		subset_(subset)
	{}
	
	struct Iterator {
	private:
		const BoundedSubsets& range_;
		int round_;
		SortedStackSubset ssi_;
		Subset subset_;

		Iterator(const BoundedSubsets& range, int round, const SortedSubsetRange& ssi, Subset subset) :
			range_(range), round_(round), ssi_(range.set_.size() + 1, ssi), subset_(subset)
		{
		}
		
		friend struct LexicalRange;
	public:
	
		Iterator& operator++() {
			int next = ssi_.empty() ? 0 : ssi_.getLargest() + 1;
			if (next < range_.set_.size()) {
				if (ssi_.size() < range_.maxSize_) {
					ssi_.insertLargest(next);
					subset_.insertLargest(range_.set_[next]);
					return *this;
				}
			}
			else {
				if (!ssi_.empty()) {
					int prev = ssi_.getLargest();
					ssi_.removeLargest(prev);
					subset_.removeLargest(range_.set_[prev]);
				}
			}
			if (ssi_.empty()) {
				++round_;
				return *this;
			}
			int prev = ssi_.getLargest();
			ssi_.removeLargest(prev);
			subset_.removeLargest(range_.set_[prev]);
			next = prev + 1;
			ssi_.insertLargest(next);
			subset_.insertLargest(range_.set_[next]);
			return *this;
		}
	
		bool operator!=(const Iterator& other) {
			return round_ != other.round_ || ssi_ != other.ssi_;
		}
	
		const Subset& operator*() {
			return subset_;
		}
	};

	Iterator begin() {
		return Iterator(range_, 0, ConstSortedSubset(), subset_);
	}

	Iterator end() {
		return Iterator(range_, 1, ConstSortedSubset(), subset_);
	}
};


#endif

