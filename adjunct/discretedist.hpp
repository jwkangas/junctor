/*
 *  Adjunct
 *  
 *  Copyright 2015 Kustaa Kangas <jwkangas(at)cs.helsinki.fi>
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

#ifndef DISCRETEDIST_HPP
#define DISCRETEDIST_HPP

//#define NDEBUG
#include <cassert>

#include "common.hpp"


/**
 * An implementation of the discrete distribution using the alias method: O(n) initialization time
 * and O(1) per-sample time.
 */
template <typename T>
class DiscreteDist {
private:
	T* prob_;
	size_t* alias_;
	std::uniform_real_distribution<T> uniDist_;
public:
	DiscreteDist(const std::vector<T>& probs)
		: uniDist_(0, probs.size())
	{
		size_t n = probs.size();
		prob_ = new T[n];
		alias_ = new size_t[n];
		
		size_t* list = new size_t[n];
		size_t s = 0;
		size_t l = n;
		for (size_t i = 0; i < n; ++i) {
			alias_[i] = i; // does not matter in theory
			prob_[i] = probs[i] * n;
			if (prob_[i] < 1)
				list[s++] = i;
			else
				list[--l] = i;
		}
		while (s > 0 && l < n) {
			size_t small = list[--s];
			size_t large = list[l];
			alias_[small] = large;
			prob_[large] = (prob_[large] + prob_[small]) - 1;
			if (prob_[large] < 1) {
				++l;
				list[s++] = large;
			}
		}
		while (l < n)
			prob_[list[l++]] = 1;
		while (s > 0)
			prob_[list[--s]] = 1;
		
		delete[] list;
	}
	
	~DiscreteDist() {
		delete[] prob_;
		delete[] alias_;
	}
	
	size_t rand() {
		T i, f;
		f = uniDist_(rng);
		assert(0 <= f && f < uniDist_.max());
		f = modf(f, &i);
		assert(0 <= i && i < uniDist_.max());
		size_t j = (size_t)i;
		if (f <= prob_[j])
			return j;
		else
			return alias_[j];
	}
};

#endif


