#ifndef SET_H
#define SET_H

#include <cstdio>
#include <cmath>
#include <cassert>
#include <cstdlib>
#include <cstring>



struct base_set
{
	virtual bool has(unsigned e) const = 0;
	virtual void set(unsigned e) = 0;
	
	bool operator& (int e) const
	{
		return has(e);
	}
	
	void print(int k) const
	{
		for (int e = 0; e < k; e++) {
			printf("%i", has(k-e-1) ? 1 : 0);
		}
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
};

template <typename T>
struct integer_set : base_set
{
	T bits;
	
	integer_set() {}
	integer_set(T bits) : bits (bits) {}
	
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
	
	void print()
	{
		print(sizeof(T) * 8);
	}
	
	void print(int k)
	{
		base_set::print(k);
	}
	
	void println()
	{
		println(sizeof(T) * 8);
	}
	
	void println(unsigned n)
	{
		base_set::println(n);
	}
	
	bool operator== (const integer_set& other) const
	{
		return bits == other.bits;
	}
	
	bool operator!= (const integer_set& other) const
	{
		return bits != other.bits;
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
};


typedef integer_set<unsigned int> uintset;



// An abstract class for iterating over sets. The iteration order depends on the subclass.
// Must be initialized by a call to init() before making instances.
template <typename Set>
struct set_iterator
{
	Set S;
	long long unsigned n_sets;	// number of sets to enumerate
	long long unsigned index;	// index of the current subset
	
	static long long unsigned binom[64+1][64+1];
	
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
	
	set_iterator(Set S, long long unsigned n_sets) : S(S), n_sets(n_sets), index(0) {}
	
	int has_next()
	{
		return index < n_sets;
	}
	
	void operator++ ()
	{
		index++;
		next();
	}
	
	virtual void next() = 0;
};

template <class Set> long long unsigned set_iterator<Set>::binom[64+1][64+1];



// enumerates sets in an interval [A,B] in the lexicographic order
template <typename Set>
struct range_iterator : public set_iterator<Set>
{
	unsigned int opt_bits[32];	// optional elements in B\A
	unsigned int obn;
	
	long long unsigned number_of_sets(int n, Set A, Set B, bool end)
	{
		assert((A | B) == B);
		return (1 << (B.cardinality(n) - A.cardinality(n))) - (end ? 0 : 1);
	}
	
	range_iterator(int n, Set A, Set B, bool start=1, bool end=1) : set_iterator<Set>(A, number_of_sets(n, A, B, end))
	{
		Set C = B ^ A;
		
		obn = 0;
		for (int i = 0; i < n; i++) {
			if (C.has(i)) opt_bits[obn++] = i;
		}
		
		if (!start) ++(*this);
	}
	
	void next()
	{
		for (unsigned int i = 0; i < obn; i++) {
			unsigned int j = opt_bits[i];
			this->S.flip(j);
			if (this->S.has(j)) return;
		}
	}
	
	Set& set()
	{
		return this->S;
	}
};





// enumerates sets of size at most k in an interval [A,B] in the lexicographic order
template <typename Set>
struct range_k_iterator : public set_iterator<Set>
{
	unsigned int opt_bits[32];	// optional elements in B\A
	unsigned int one_bits[32];	// optional bits currently set to one
	unsigned int opt_n;			// number of optional bits
	unsigned int one_n;			// number of current optional one bits
	unsigned int opt_k;			// cardinality bound on the optional elements
	
	long long unsigned number_of_sets(unsigned n, unsigned k, Set A, Set B, bool end)
	{
		assert((A | B) == B);
		
		long long card_A = A.cardinality(n);
		long long card_B = B.cardinality(n);
		long long card_C = card_B - card_A;
		
		opt_k = k - card_A;
		
		long long unsigned sets = 0;
		for (unsigned i = 0; i <= opt_k; i++) {
			sets += set_iterator<Set>::binom[card_C][i];
		}
		
		if (!end && k >= card_B) sets--;
		
		return sets;
	}
	
	range_k_iterator(unsigned n, unsigned k, Set A, Set B, bool start=1, bool end=1) : set_iterator<Set>(A, number_of_sets(n, k, A, B, end))
	{
		Set C = B ^ A;
		
		for (unsigned int i = 0; i < opt_k; i++) one_bits[i] = 0;
		
		opt_n = 0;
		one_n = 0;
		for (unsigned i = 0; i < n; i++) {
			if (C.has(i)) opt_bits[opt_n++] = i;
		}
		
		if (!start) ++(*this);
	}
	
	// flips the i'th optional bit
	void flip_opt(unsigned i)
	{
		this->S.flip(opt_bits[i]);
	}
	
	// returns the i'th optional bit
	bool opt_bit(unsigned i)
	{
		return this->S.has(opt_bits[i]);
	}
	
	void next()
	{
		if (set_iterator<Set>::index == set_iterator<Set>::n_sets) return;
		
		// starting from the first element, or the first 1 bit if cardinality is full
		unsigned i = (one_n == opt_k) ? one_bits[one_n-1] : 0;
		
		// first flip all consecutive 1s to 0
		while (opt_bit(i)) {
			flip_opt(i);
			one_n--;
			i++;
		}
		
		// then flip the first 0 after them to 1
		flip_opt(i);
		one_bits[one_n] = i;
		one_n++;
	}
	
	Set& set()
	{
		return this->S;
	}
};






// SubsetArray stores a value T for each pair of disjoint subsets of n elements

template <typename T>
struct SubsetArray
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
	
	SubsetArray(unsigned n, unsigned w) : n(n)
	{
		long long unsigned x_size = 1 << n;
		long long unsigned y_size = estimate(n, w);
		
		values = new T*[x_size];
		assert(values != NULL);
		array = new T[y_size];
		assert(array != NULL);
		
		for (unsigned i = 0; i < y_size; i++) {
			array[i] = -DBL_MAX;
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
		unsigned s = 1;		// s = 1 << i
		unsigned r = 1;		// r = 1 << j
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
	
	~SubsetArray()
	{
		delete [] values;
		delete [] array;
	}
};


#endif
