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

#include "common.hpp"
#include "discretedist.hpp"


struct SampleCache
{
	Set *samples;	// cached samples
	unsigned size;	// number of cached samples
	unsigned n;		// number of samples to draw in next sampling
	
	SampleCache() : samples(NULL), size(0), n(1) {}
	
	~SampleCache()
	{
		delete [] samples;
	}
	
	// adds a set X to the cache
	void add(Set X)
	{
		samples[size] = X;
		size++;
	}
	
	// gets (and removes) a set from the cache
	Set consume()
	{
		size--;
		return samples[size];
	}
	
	// builds a cache of n samples from sets with the respective probabilities
	void build(std::vector<double> &probs, std::vector<Set> &sets)
	{
		// reallocate cache buffer
		delete [] samples;
		samples = new Set[n];
		
		// create an alias table of the probabilities
		DiscreteDist<double> alias(probs);
		
		// draw n samples from the table and map them to respective sets
		for (unsigned i = 0; i < n; i++) add(sets[alias.rand()]);
		assert(size == n);
		
		// double the number of samples to draw next time
		n *= 2;
	}
};

DisjointPairArray<SampleCache*> *f_samples, *g_samples, *h_samples;


SampleCache *get_sample_cache(DisjointPairArray<SampleCache*> *samples, Set X, Set Y)
{
	SampleCache *cache = samples->get(X.bits, Y.bits);
	if (cache == NULL) {
		cache = new SampleCache();
		samples->set(X.bits, Y.bits, cache);
	}
	return cache;
}

bool free_sample_cache(DisjointPairArray<SampleCache*> *samples, Set X, Set Y)
{
	SampleCache *cache = samples->get(X.bits, Y.bits);
	if (cache == NULL) return false;
	delete cache;
	samples->set(X.bits, Y.bits, NULL);
	return true;
}





double compute_sum_f(Set S, Set R);
double compute_sum_g(Set C, Set U);
double compute_sum_h(Set C, Set R);


void rebuild_cache_h(Set C, Set R, SampleCache *cache)
{
	double total = compute_sum_h(C, R);
	double sum_score = -INFTY;
	
	std::vector<double> probs;
	std::vector<Set> sets;
	
	H_ITERATE(it) {
		Set S = it.set();
		
		double score_s = local_score(S);
		double score_f = compute_sum_f(S, R);
		double score = score_f - score_s;
		
		sum_score = logsum(sum_score, score - total);
		
		probs.push_back(exp(score - total));
		sets.push_back(S);
	}
	
	cache->build(probs, sets);
}

void rebuild_cache_g(Set C, Set U, SampleCache *cache)
{
	double total = compute_sum_g(C, U);
	double sum_score = -INFTY;
	
	std::vector<double> probs;
	std::vector<Set> sets;
	
	G_ITERATE(it) {
		Set R = it.set();
		
		double score_h = compute_sum_h(C, R);
		double score_g = compute_sum_g(C, U ^ R);
		double score = score_h + score_g;
		
		sum_score = logsum(sum_score, score - total);
		
		probs.push_back(exp(score - total));
		sets.push_back(R);
	}
	
	cache->build(probs, sets);
}

void rebuild_cache_f(Set S, Set R, SampleCache *cache)
{
	double total = compute_sum_f(S, R);
	double sum_score = -INFTY;
	
	std::vector<double> probs;
	std::vector<Set> sets;
	
	F_ITERATE(it) {
		Set D = it.set();
		Set C = S | D;
		
		double score_c = local_score(C);
		double score_g = compute_sum_g(C, R ^ D);
		double score = score_c + score_g;
		
		sum_score = logsum(sum_score, score - total);
		
		probs.push_back(exp(score - total));
		sets.push_back(D);
	}
	
	cache->build(probs, sets);
}



TreeNode<Set> *sample_f_adaptive(Set S, Set R, TreeNode<Set> *node);
void sample_h_adaptive(Set C, Set R, TreeNode<Set> *node);
void sample_g_adaptive(Set C, Set U, TreeNode<Set> *node);


void sample_h_adaptive(Set C, Set R, TreeNode<Set> *node)
{
	SampleCache *cache = get_sample_cache(h_samples, C, R);
	if (cache->size == 0) rebuild_cache_h(C, R, cache);
	
	Set S = cache->consume();
	sample_f_adaptive(S, R, node);
}

void sample_g_adaptive(Set C, Set U, TreeNode<Set> *node)
{
	if (U.is_empty()) return;
	
	SampleCache *cache = get_sample_cache(g_samples, C, U);
	if (cache->size == 0) rebuild_cache_g(C, U, cache);
	
	Set R = cache->consume();
	sample_h_adaptive(C, R, node);
	sample_g_adaptive(C, U ^ R, node);
}

TreeNode<Set> *sample_f_adaptive(Set S, Set R, TreeNode<Set> *node)
{
	SampleCache *cache = get_sample_cache(f_samples, S, R);
	if (cache->size == 0) rebuild_cache_f(S, R, cache);
	
	Set D = cache->consume();
	Set C = S | D;
	
	TreeNode<Set> *child = new TreeNode<Set>(C, S);
	if (node != NULL) node->add(child);
	sample_g_adaptive(C, R ^ D, child);
	return child;
}

TreeNode<Set> *sample_adaptive()
{
	return sample_f_adaptive(Set::empty(N), Set::complete(N), (TreeNode<Set>*)NULL);
}




void free_cache_h(Set C, Set R);
void free_cache_g(Set C, Set U);
void free_cache_f(Set S, Set R);


void free_cache_h(Set C, Set R)
{
	if (!free_sample_cache(h_samples, C, R)) return;
	
	H_ITERATE(it) {
		Set S = it.set();
		free_cache_f(S, R);
	}
}

void free_cache_g(Set C, Set U)
{
	if (U.is_empty()) return;
	
	if (!free_sample_cache(g_samples, C, U)) return;
	
	G_ITERATE(it) {
		Set R = it.set();
		free_cache_h(C, R);
		free_cache_g(C, U ^ R);
	}
}

void free_cache_f(Set S, Set R)
{
	if (!free_sample_cache(f_samples, S, R)) return;
	
	F_ITERATE(it) {
		Set D = it.set();
		Set C = S | D;
		free_cache_g(C, R ^ D);
	}
}





void sampling_adaptive_init()
{
	f_samples = new DisjointPairArray<SampleCache*>(N, N, NULL);
	g_samples = new DisjointPairArray<SampleCache*>(N, N, NULL);
	h_samples = new DisjointPairArray<SampleCache*>(N, N, NULL);
}

void sampling_adaptive_uninit()
{
	free_cache_f(Set::empty(N), Set::complete(N));
	
	delete f_samples;
	delete g_samples;
	delete h_samples;
}


