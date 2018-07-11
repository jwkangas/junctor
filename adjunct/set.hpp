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

#ifndef SET_H
#define SET_H

#include <cstdio>
#include <cmath>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdint>



// a convenient wrapper for representing sets as integers
template <typename T>
struct integer_set
{
	T bits;
	
	integer_set() {}
	integer_set(T bits) : bits (bits) {}
	
	bool operator& (int e) const
	{
		return has(e);
	}
	
	unsigned cardinality(int n) const
	{
		unsigned count = 0;
		for (int i = 0; i < n; i++) {
			if (has(i)) count++;
		}
		return count;
	}
	
	bool operator[] (unsigned e) const
	{
		return has(e);
	}
	
	// returns the index of the first one (among the first k), or k if no such bit
	unsigned first(unsigned k) const
	{
		unsigned i;
		for (i = 0; i < k; i++) {
			if (has(i)) break;
		}
		return i;
	}
	
	unsigned get_list(unsigned k, int *list) const
	{
		unsigned n = 0;
		for (unsigned i = 0; i < k; i++) {
			if (has(i)) {
				list[n] = i;
				n++;
			}
		}
		return n;
	}
	
	integer_set operator& (integer_set S)
	{
		return integer_set(bits & S.bits);
	}
	
	integer_set operator| (integer_set S)
	{
		return integer_set(bits | S.bits);
	}
	
	integer_set operator^ (unsigned e)
	{
		return integer_set(bits ^ sing(e));
	}
	
	integer_set operator| (unsigned e)
	{
		return integer_set(bits | sing(e));
	}
	
	integer_set operator^ (integer_set S)
	{
		return integer_set(bits ^ S.bits);
	}
	
	void operator&= (integer_set S)
	{
		bits &= S.bits;
	}
	
	void operator|= (integer_set S)
	{
		bits |= S.bits;
	}
	
	bool has(unsigned e) const
	{
		return bits & sing(e);
	}
	
	void set(unsigned e)
	{
		bits |= sing(e);
	}
	
	void flip(unsigned e)
	{
		bits ^= sing(e);
	}
	
	void operator^= (unsigned e)
	{
		flip(e);
	}
	
	static T sing(unsigned e)
	{
		return (T)1 << e;
	}
	
	static integer_set empty(unsigned)
	{
		return integer_set(0);
	}
	
	static integer_set complete(unsigned n)
	{
		return ((T)-1) >> (sizeof(T)*8 - n);
	}
	
	bool operator== (const integer_set& other) const
	{
		return bits == other.bits;
	}
	
	bool operator!= (const integer_set& other) const
	{
		return bits != other.bits;
	}
	
	bool subsetof(integer_set& other) const
	{
		return (other.bits | bits) == other.bits;
	}
	
	unsigned count(unsigned n) const
	{
		unsigned c = 0;
		for (unsigned k = 0; k < n; k++) {
			if (has(k)) c++;
		}
		return c;
	}
	
	bool is_empty() const
	{
		return bits == 0;
	}
	
	void print(int k) const
	{
		for (int e = 0; e < k; e++) {
			printf("%i", has(k-e-1) ? 1 : 0);
		}
	}
	
	void print()
	{
		print(sizeof(T) * 8);
	}
	
	void println()
	{
		println(sizeof(T) * 8);
	}
	
	void lprint(unsigned k) const
	{
		bool empty = true;
		for (unsigned e = 0; e < k; e++) {
			if (has(e)) {
				printf("%c", 'A'+e);
				empty = false;
			} else {
				printf(" ");
			}
		}
		if (empty) printf("Ø");
	}
	
	void rprint(unsigned k) const
	{
		bool empty = true;
		printf("{");
		for (unsigned e = 0; e < k; e++) {
			if (!has(e)) continue;
			if (!empty) printf(",");
			printf("%i", e);
			empty = false;
		}
		printf("}");
	}
	
	void rcat(char *str, unsigned k) const
	{
		bool empty = true;
		strcat(str, "{");
		for (unsigned e = 0; e < k; e++) {
			if (!has(e)) continue;
			if (!empty) strcat(str, ",");
			char buf[256];
			sprintf(buf, "%i", e);
			strcat(str, buf);
			empty = false;
		}
		strcat(str, "}");
	}
	
	void println(int k) const
	{
		print(k);
		putchar('\n');
	}
	
	void lprintln(int k) const
	{
		lprint(k);
		putchar('\n');
	}
	
	void rprintln(int k) const
	{
		rprint(k);
		putchar('\n');
	}
};


typedef integer_set<unsigned int> uintset;

#define MAX_SET_SIZE 32




// iterates over sets in a given range [A,B] in colexicographic order
template <typename Set>
struct range_iterator
{
	// total number of sets to iterate over
	long long unsigned n_sets;
	
	// index of the current set in iteration
	long long unsigned index;
	
	// current set in iteration
	Set S;
	
	// positions of free bits (those that change in iteration)
	int free_bits[MAX_SET_SIZE];
	
	// number of free bits, |B\A|
	int free_n;
	
	// n:          number of elements in the universe
	// A, B:       subsets of the universe such that A \subseteq B
	// include_A:  include the endpoint A? (true by default)
	// include_B:  include the endpoint B? (true by default)
	range_iterator(int n, Set A, Set B, bool include_A=1, bool include_B=1)
	{
		assert((A | B) == B);
		
		// count the number of sets
		n_sets = 1 << (B.cardinality(n) - A.cardinality(n));
		if (!include_B) n_sets--;
		
		// initialize
		index = 0;
		S = A;
		
		// build a mapping to free bits
		Set C = B ^ A;
		free_n = 0;
		for (int i = 0; i < n; i++) {
			if (C.has(i)) free_bits[free_n++] = i;
		}
		
		// if A is not included, skip the first set
		if (!include_A) ++(*this);
	}
	
	void operator++ ()
	{
		index++;
		
		// to produce the next set in iteration,
		// flip free bits in order until the first 0 is flipped to 1
		for (int i = 0; i < free_n; i++) {
			int j = free_bits[i];
			this->S.flip(j);
			if (this->S.has(j)) return;
		}
	}
	
	Set& set()
	{
		return this->S;
	}
	
	int has_next() const
	{
		return index < n_sets;
	}
};




// iterates over sets of given maximum size in a given range [A,B] in colexicographic order
template <typename Set>
struct range_k_iterator
{
	// precomputed binomial coefficients
	static long long unsigned binom[MAX_SET_SIZE+1][MAX_SET_SIZE+1];
	
	static long double factorial(unsigned n)
	{
		long double fac = 1;
		for (unsigned i = 2; i <= n; i++) fac *= i;
		return fac;
	}
	
	static void init(unsigned size)
	{
		for (unsigned i = 0; i <= size; i++) {
			for (unsigned j = 0; j <= i; j++) {
				binom[i][j] = (long long unsigned)(factorial(i) / factorial(j) / factorial(i-j));
			}
		}
	}
	
	// returns the number of subsets of n elements having size at most k
	static long long unsigned subsets_of_size_at_most(int n, int k)
	{
		long long unsigned sets = 0;
		for (int i = 0; i <= k; i++) sets += binom[n][i];
		return sets;
	}
	
	// total number of sets to iterate over
	long long unsigned n_sets;
	
	// index of the current set in iteration
	long long unsigned index;
	
	// current set in iteration
	Set S;
	
	// positions of free bits (those that change in iteration)
	int free_bits[MAX_SET_SIZE];
	
	// maximum number of free bits that can be 1 at the same time
	int free_max;
	
	// current number of free 1 bits
	int one_n;
	
	// positions of free 1 bits (relative to 1 bits, not all bits)
	int one_bits[MAX_SET_SIZE];
	
	// n:          number of elements in the universe
	// k:          maximum size of sets to iterate over
	// A, B:       subsets of the universe such that A \subseteq B
	// include_A:  include the endpoint A? (true by default)
	// include_B:  include the endpoint B? (true by default)
	// note: endpoints are never included if their size exceeds k
	range_k_iterator(int n, int k, Set A, Set B, bool start=1, bool end=1)
	{
		assert((A | B) == B);
		
		int card_A = A.cardinality(n);
		int card_B = B.cardinality(n);
		int card_C = card_B - card_A;
		
		// the maximum number of free 1 one bits is bounded by k-|A| and |B|-|A|
		free_max = k - card_A;
		if (card_C < free_max) free_max = card_C;
		
		// the total number of sets to be iterated over
		n_sets = subsets_of_size_at_most(card_C, free_max);
		if (!end && k >= card_B) n_sets--;
		
		// initialize
		index = 0;
		S = A;
		
		// get the positions of free bits
		(B ^ A).get_list(n, free_bits);
		
		// initially all free bits are 0
		one_n = 0;
		
		// if A not included in the range, skip it
		if (!start) ++(*this);
	}
	
	// flips the i'th optional bit
	void flip_opt(int i)
	{
		S.flip(free_bits[i]);
	}
	
	// returns the i'th optional bit
	bool opt_bit(int i) const
	{
		return S.has(free_bits[i]);
	}
	
	void next()
	{
		if (index == n_sets) return;
		
		// to produce the next set in iteration, choose the first free bit,
		// or the first free 1 bit in case we have maximum number of free bits set to 1
		int i = (one_n == free_max) ? one_bits[one_n-1] : 0;
		
		// starting from this bit, flip all bits until the first 0 is flipped to 1
		
		// 1. first flipping 1s to 0s
		while (opt_bit(i)) {
			flip_opt(i);
			one_n--;
			i++;
		}
		
		// 2. then flipping the first 0 to 1
		flip_opt(i);
		one_bits[one_n] = i;
		one_n++;
	}
	
	Set& set()
	{
		return this->S;
	}
	
	int has_next() const
	{
		return index < n_sets;
	}
	
	void operator++ ()
	{
		index++;
		next();
	}
};

template <class Set> long long unsigned range_k_iterator<Set>::binom[MAX_SET_SIZE+1][MAX_SET_SIZE+1];







// stores a value T for each pair of disjoint subsets of n elements
template <typename T>
struct DisjointPairArray
{
	unsigned n;
	T **values;
	T *array;
	
	static long long unsigned estimate(unsigned n, unsigned w)
	{
		long long unsigned x_size = 1 << n;
		
		long long unsigned y_size = 0;
		for (unsigned i = 0; i < x_size; i++) {
			unsigned k = uintset(i).cardinality(n);
			if (k > w) continue;
			y_size += (1 << (n - k));
		}
		
		return y_size;
	}
	
	DisjointPairArray(unsigned n, unsigned w, T initial) : n(n)
	{
		long long unsigned x_size = 1 << n;
		long long unsigned y_size = estimate(n, w);
		
		values = new T*[x_size];
		assert(values != NULL);
		array = new T[y_size];
		assert(array != NULL);
		
		for (unsigned i = 0; i < y_size; i++) {
			array[i] = initial;
		}
		
		T *p = array;
		for (unsigned i = 0; i < x_size; i++) {
			unsigned k = uintset(i).cardinality(n);
			if (k > w) {
				values[i] = NULL;
				continue;
			}
			values[i] = p;
			p += (1 << (n - k));
		}
	}
	
	// maps y to a "short index" using only n - b bits where b is the number of 1s in x
	unsigned index(unsigned x, unsigned y)
	{
		unsigned ind = 0;	// short index of y
		unsigned j = 0;		// position in the index of y
		unsigned s = 1;		// we maintain s = 1 << i
		unsigned r = 1;		// we maintain r = 1 << j
		for (unsigned i = 0; i < n; i++, s <<= 1) {
			if (x & s) continue;				// skip 1-bits of x
			ind |= (r & ((y & s) >> (i - j)));	// if (y & s) ind |= r;
			j++;
			r <<= 1;
		}
		
		return ind;
	}
	
	T get(unsigned x, unsigned y)
	{
		return values[x][index(x, y)];
	}
	
	void set(unsigned x, unsigned y, T value)
	{
		values[x][index(x, y)] = value;
	}
	
	~DisjointPairArray()
	{
		delete [] values;
		delete [] array;
	}
};


#endif
