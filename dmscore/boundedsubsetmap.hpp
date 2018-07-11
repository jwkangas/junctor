/*
 *  BoundedSubsetMap class
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

#include <vector>

#include "common.hpp"
#include "sortedsubset.hpp"
#include "subsets.hpp"

#ifndef BOUNDEDSUBSETMAP_HPP
#define BOUNDEDSUBSETMAP_HPP

template <typename T>
class BoundedSubsetMap {
private:
	ConstSortedSubset groundSet_;
	std::vector<int> elementIndices_;
	const int maxSubsetSize_;

	struct Datum {
		Datum* parent;
		Datum* child0;
		T value;
	};
	Datum* root_;

	void buildRecursive(Datum* x, int depth, int nextIndex, Datum*& freeDatums) {
		//for (int i = 0; i < setSize; ++i)
		//	printf("%d", inSet[i]);
		//printf(" => %d \n", x);

		if (depth == 0) {
		//if (depth == 0 || nextIndex == groundSet_.size()) {
			x->child0 = nullptr;
			return;
		}
		else {
			x->child0 = freeDatums - nextIndex;
			freeDatums += groundSet_.size() - nextIndex;
			for (int i = nextIndex; i < groundSet_.size(); ++i) {
				Datum* child = x->child0 + i;
				child->parent = x;
				buildRecursive(child, depth - 1, i + 1, freeDatums);
			}
		}

	}

	Datum* getDatum(const SortedSubsetRange& subset) const {
		assert(subset.size() <= maxSubsetSize_);
		Datum* x = root_;
		for (int i = 0; i < subset.size(); ++i)
			x = x->child0 + elementIndices_[subset[i]];
		return x;
	}

	template <typename SubsetElements>
	void backTrackElementsRecursive(Datum* x, SubsetElements& subset) const {
		if (x != root_) {
			Datum* parent = x->parent;
			backTrackElementsRecursive(parent, subset);
			subset.insertLargest(groundSet_[x - parent->child0]);
		}
	}

	template <typename SubsetElements>
	void backTrackElements(Datum* x, SubsetElements& subset) const {
		subset.clear();
		backTrackElementsRecursive(x, subset);
		/*int size = 0;
		while (x != root_) {
			x = x->parent;
			++size;
		}
		subset.resize(size);
		int i = 0;
		while (x != root_) {
			Datum* parent = x->parent;
			subset[i] = groundSet_[x - parent->child0];
			x = parent;
			++i;
		}*/
	}
		
	BoundedSubsetMap(const BoundedSubsetMap&) = delete; // disable copying
	BoundedSubsetMap& operator=(const BoundedSubsetMap&) = delete; // disable copying
public:
	BoundedSubsetMap(const ConstSortedSubset& groundSet, int maxSubsetSize) :
		groundSet_(groundSet),
		//elementIndices_(max(groundSet), -1),
		elementIndices_(groundSet.getLargest() + 1, -1),
		maxSubsetSize_(maxSubsetSize)
	{
		for (int i = 0; i < groundSet_.size(); ++i)
			elementIndices_[groundSet_[i]] = i;
		unsigned long nSubsets = numBoundedSubsets(groundSet_.size(), maxSubsetSize_);
		Datum* freeDatums = new Datum[nSubsets];
		root_ = freeDatums;
		root_->parent = nullptr;
		freeDatums += 1;
		buildRecursive(root_, maxSubsetSize_, 0, freeDatums);
		
	}
	
	~BoundedSubsetMap() {
		delete[] root_;
	}
	
	class SubsetIndex {
	private:
		Datum* datum_;
		SubsetIndex(Datum* datum) : datum_(datum) {}
	public:
		bool operator==(const SubsetIndex& other) const {
			return datum_ == other.datum_;
		}
		friend class BoundedSubsetMap;
	};

	template <typename SubsetElements>
	void getElements(const SubsetIndex& index, SubsetElements& subset) const {
		backTrackElements(index.datum_, subset);
	}

	SubsetIndex getIndex(const SortedSubsetRange& subset) const {
		return SubsetIndex(getDatum(subset));
	}

	SubsetIndex getEmptyIndex() const {
		return SubsetIndex(root_);
	}

	unsigned long numSubsets() const {
		return numBoundedSubsets(groundSet_.size(), maxSubsetSize_);
	}

	class Subset {
	private:
		const BoundedSubsetMap& map_;
		Datum* datum_;
	public:
		Subset(const BoundedSubsetMap& map) :
			map_(map),
			datum_(map.root_)
		{}

		Subset(const BoundedSubsetMap& map, const SubsetIndex& index) :
			map_(map),
			datum_(index.datum_)
		{}
		
		bool canInsert() const {
			return datum_->child0 != nullptr;
		}
		
		bool isEmpty() const {
			return datum_->parent == nullptr;
		}

		void insertLargest(int elem) {
			datum_ = datum_->child0 + map_.elementIndices_[elem];
		}
		void removeLargest(int elem) {
			assert(datum_->parent != nullptr);
			assert(getLargest() == elem);
			datum_ = datum_->parent;
		}
		int getLargest() const {
			assert(datum_->parent != nullptr);
			return map_.groundSet_[datum_ - datum_->parent->child0];
		}

		SubsetIndex getIndex() const {
			return SubsetIndex(datum_);
		}

		template <typename SubsetElements>
		void getElements(SubsetElements& subset) const {
			map_.backTrackElements(datum_, subset);
		}

		friend class BoundedSubsetMap;
	};

	T& operator[] (const Subset& subset) {
		return subset.datum_->value;
	}

	const T& operator[] (const Subset& subset) const {
		return subset.datum_->value;
	}

	T& operator[] (const SubsetIndex& subsetIndex) {
		return subsetIndex.datum_->value;
	}

	const T& operator[] (const SubsetIndex& subsetIndex) const {
		return subsetIndex.datum_->value;
	}

	T& operator[] (const SortedSubsetRange& subset) {
		Datum* x = getDatum(subset);
		return x->value;
	}

	const T& operator[] (const SortedSubsetRange& subset) const {
		Datum* x = getDatum(subset);
		return x->value;
	}


	template <typename KeySubset, typename Value, typename SubsetMapRef>
	struct RangeImpl {
	private:
		using SubsetPair = SetPair<KeySubset, Subset>;
		using SubsetRange = BoundedSubsets::LexicalRange<SubsetPair>;
		
		SubsetMapRef map_;
		SubsetRange subsetRange_;
	
	public:
		RangeImpl(SubsetMapRef map, const ConstSortedSubset& set,
				const KeySubset& keySubset) :
			map_(map),
			subsetRange_(BoundedSubsets(set, map_.maxSubsetSize_),
				SubsetPair(keySubset, Subset(map_)))
		{
		}
		
		using KeyValuePair = std::pair<const KeySubset&, Value>;

		struct Iterator {
		private:
			const RangeImpl& range_;
			typename SubsetRange::Iterator subsetIter_;
		public:
			Iterator(const RangeImpl& range,
					const typename SubsetRange::Iterator& subsetIter) :
				range_(range), subsetIter_(subsetIter)
			{}
			
			Iterator& operator++() {
				++subsetIter_;
				return *this;
			}
			
			bool operator!=(const Iterator& other) {
				return subsetIter_ != other.subsetIter_;
			}
			
			KeyValuePair operator*() {
				return KeyValuePair((*subsetIter_).first,
						range_.map_[(*subsetIter_).second]);
			}
		};
		
		Iterator begin() {
			return Iterator(*this, subsetRange_.begin());
		}
		
		Iterator end() {
			return Iterator(*this, subsetRange_.end());
		}
		
	};
	
	template <typename KeySubset>
	using Range = RangeImpl<KeySubset, T&, BoundedSubsetMap&>;

	template <typename KeySubset>
	using ConstRange = RangeImpl<KeySubset, const T&, const BoundedSubsetMap&>;

	template <typename Subset>
	Range<Subset> forSubsetsOf(const ConstSortedSubset& set,
			const Subset& subsetInitializer) {
		return Range<Subset>(*this, set, subsetInitializer);
	}
	template <typename Subset>
	ConstRange<Subset> forSubsetsOf(const ConstSortedSubset& set,
			const Subset& subsetInitializer) const {
		return ConstRange<Subset>(*this, set, subsetInitializer);
	}

	template <typename Subset>
	Range<Subset> forSubsets(const Subset& subsetInitializer) {
		return Range<Subset>(*this, groundSet_, subsetInitializer);
	}
	template <typename Subset>
	ConstRange<Subset> forSubsets(const Subset& subsetInitializer) const {
		return ConstRange<Subset>(*this, groundSet_, subsetInitializer);
	}

	Range<Subset> forSubsets() {
		return Range<Subset>(*this, groundSet_, Subset(*this));
	}
	ConstRange<Subset> forSubsets() const {
		return ConstRange<Subset>(*this, groundSet_, Subset(*this));
	}
};


#endif
