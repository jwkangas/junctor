/*
 *  Functions for computing scores
 *  
 *  Copyright 2011-2015 Teppo Niinimäki <teppo.niinimaki(at)helsinki.fi>
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


#include <cmath>
//#include <unordered_map>

#include "data.hpp"
#include "sortedsubset.hpp"

#ifndef SUBSETSCORE_HPP
#define SUBSETSCORE_HPP


class SubsetScore {
public:
	virtual ~SubsetScore() {};
	virtual double compute(int nValues, int* counts) const = 0;
	//virtual double compute(const std::vector<int>& set, const ADTree* adTree) const = 0;
	virtual double mapCount(double arity, int count) const = 0;
};


/**
 * BDeu score function.
 */
class BDeuSubsetScore : public SubsetScore {
private:
	double ess_;
	
	class MapFun {
	private:
		double pseudoCount_;
	public:
		MapFun(double pseudoCount) : pseudoCount_(pseudoCount) {}
		double operator()(int count) const {
			return lgamma(count + pseudoCount_) - lgamma(pseudoCount_);
		}
	};
public:
	BDeuSubsetScore(double ess) {
		ess_ = ess;
	}
	double compute(int nValues, int* counts) const {
		double score = 0;
		double pseudoCount = ess_ / nValues;
		int cumCount = 0;
		for (int v = 0; v < nValues; ++v) {
			int c = counts[v];
			score += lgamma(c + pseudoCount) - lgamma(pseudoCount);
			cumCount += c;
		}
		score += lgamma(ess_) - lgamma(cumCount + ess_);
		return score;
	}
	
	/*double compute(const std::vector<int>& set, const ADTree* adTree) const {
		double nValues = adTree->getTotalArity(set);
		MapFun mapFun(ess_ / nValues);
		double score = adTree->mapSumCounts(mapFun, set);
		score += lgamma(ess_) - lgamma(adTree->getNumSamples() + ess_);
		return score;
	}*/
	
	double mapCount(double arity, int count) const {
		double pseudoCount = ess_ / arity;
		return lgamma(count + pseudoCount) - lgamma(pseudoCount);
	}
};

/*class BDeuSubsetScoreCached : public BDeuSubsetScore {
private:
	struct cacheHash {
		size_t operator()(const std::pair<double, int> &pair) const {
			return std::hash<double>()(pair.first) ^ std::hash<int>()(pair.second);
		}
	};

	mutable std::unordered_map<std::pair<double, int>, double, cacheHash> cache;
	
public:
	BDeuSubsetScoreCached(double ess)
		: BDeuSubsetScore(ess)
	{
	}
	
	double mapCount(double arity, int count) const {
		auto score = cache.find(std::make_pair(arity, count));
		if (score != cache.end()) {
			return score->second;
		}
		else {
			double score = BDeuSubsetScore::mapCount(arity, count);
			cache[std::make_pair(arity, count)] = score;
			return score;
		}
	}
};*/




/*double computeMNScore(const DataView* dataView, const std::vector<int>& set, const SubsetScore* scoreFun) {
	// get counts
	int* counts = dataView->getCounts(set);
	
	// compute score
	int nValues = 1;
	for (int i = 0; i < set.size(); ++i)
		nValues *= dataView->getArity(set[i]);
	double score = scoreFun->compute(nValues, counts);
	delete[] counts;
	return score;
}/**/



/**
 * Computes the scores for a downwards closed collection of node subsets.
 */
template <typename T, typename SubsetMap, typename SubsetGrowTestFun>
void computeSubsetScores(const CategoricalData<T>* data, const SortedSubsetRange& vars,
		SubsetGrowTestFun canGrow, SubsetScore* scoreFun, SubsetMap& scores) {
	
	int n = vars.size();
	
	// get arities
	std::vector<int> arities(n);
	for (int i = 0; i < n; ++i)
		arities[i] = data->getArity(vars[i]);
	
	// start from the empty subset
	typename SubsetMap::Subset subset(scores);
	
	// recursively maps and sums zero counts
	/*std::function<void (double, int, double)>
	mapSumZerosRecursive = [&] (double totalArity, int nextVar, double zeroArity) {
		scores[subset] += zeroArity * scoreFun->mapCount(totalArity, 0);
		if (canGrow(subset) && nextVar < n) {
			// select next variable
			for (int i = nextVar; i < n; ++i) {
				int v = vars[i];
				subset.insertLargest(v);
				mapSumZerosRecursive(totalArity * arities[i], i + 1,
						zeroArity * arities[i]);
				subset.removeLargest(v);
			}
		}
	};*/

	// recursively maps and sums counts (specialization for zero counts above)
	std::function<void (double, int, const std::vector<int>&)>
	mapSumCountsRecursive = [&] (double totalArity, int nextVar,
			const std::vector<int>& records) {
		scores[subset] += scoreFun->mapCount(totalArity, records.size());
		if (canGrow(subset) && nextVar < n) {
			// select next variable
			for (int i = nextVar; i < n; ++i) {
				int v = vars[i];
				subset.insertLargest(v);
				// partition records by value
				std::vector<std::vector<int>> childRecords(arities[i]);
				for (int r : records) {
					T val = (*data)(v, r);
					childRecords[val].push_back(r);
				}
				double childTotalArity = totalArity * arities[i];
				// continue recursively for each value
				for (int val = 0; val < arities[i]; ++val) {
					if (!childRecords[val].empty())
						mapSumCountsRecursive(childTotalArity, i + 1, childRecords[val]);
					//else
					//	mapSumZerosRecursive(childTotalArity, i + 1, 1);
						
				}
				subset.removeLargest(v);
			}
		}
	};

	// initialize the scores
	for (auto subsetScore : scores.forSubsets()) {
		auto& score = subsetScore.second;
		score = -scoreFun->mapCount(1, data->getNumSamples());
	}
	
	// call the recursion, starting from empty set and all records
	std::vector<int> allRecords(data->getNumSamples());
	for (int r = 0; r < data->getNumSamples(); ++r)
		allRecords[r] = r;
	mapSumCountsRecursive(1, 0, allRecords);
}





#endif

