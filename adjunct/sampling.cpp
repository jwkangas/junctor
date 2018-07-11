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

#include <ctime>

#include "common.hpp"
#include "discretedist.hpp"


double compute_sum_f(Set S, Set R);
double compute_sum_g(Set C, Set U);
double compute_sum_h(Set C, Set R);


double compute_sum_h(Set C, Set R)
{
	double cached = h_values->get(C.bits, R.bits);
	if (cached != -INFTY) return cached;
	
	double sum_score = -INFTY;
	
	H_ITERATE(it) {
		Set S = it.set();
		
		double score_s = local_score(S);
		double score_f = compute_sum_f(S, R);
		double score = score_f - score_s;
		
		sum_score = logsum(sum_score, score);
	}
	
	h_values->set(C.bits, R.bits, sum_score);
	return sum_score;
}


double compute_sum_g(Set C, Set U)
{
	double cached = g_values->get(C.bits, U.bits);
	if (cached != -INFTY) return cached;
	
	if (U.is_empty()) {
		g_values->set(C.bits, U.bits, 0.0);
		return 0.0;
	}
	
	double sum_score = -INFTY;
	
	G_ITERATE(it) {
		Set R = it.set();
		
		double score_h = compute_sum_h(C, R);
		double score_g = compute_sum_g(C, U ^ R);
		double score = score_h + score_g;
		
		sum_score = logsum(sum_score, score);
	}
	
	g_values->set(C.bits, U.bits, sum_score);
	return sum_score;
}


double compute_sum_f(Set S, Set R)
{
	double cached = f_values->get(S.bits, R.bits);
	if (cached != -INFTY) return cached;
	
	double sum_score = -INFTY;
	
	F_ITERATE(it) {
		Set D = it.set();
		Set C = S | D;
		
		double score_c = local_score(C);
		double score_g = compute_sum_g(C, R ^ D);
		double score = score_c + score_g;
		
		sum_score = logsum(sum_score, score);
	}
	
	f_values->set(S.bits, R.bits, sum_score);
	return sum_score;
}



TreeNode<Set> *sample_naive();
void sampling_adaptive_init();
void sampling_adaptive_uninit();
TreeNode<Set> *sample_adaptive();



struct Sampler
{
	virtual ~Sampler() {};
	virtual TreeNode<Set> *sample() = 0;
};

struct NaiveSampler : public Sampler
{
	TreeNode<Set> *sample()
	{
		return sample_naive();
	}
};

struct AdaptiveSampler : public Sampler
{
	AdaptiveSampler()
	{
		sampling_adaptive_init();
	}
	
	~AdaptiveSampler()
	{
		sampling_adaptive_uninit();
	}
	
	TreeNode<Set> *sample()
	{
		return sample_adaptive();
	}
};




void sample(int n_samples, Sampler *sampler)
{
	// sum of the weights of all sampled partition trees
	double weight_total = 0.0;
	
	// sum of the weights of sampled trees containing a certain edge
	int edge_graphs[MAX_SET_SIZE][MAX_SET_SIZE];
	double edge_weights[MAX_SET_SIZE][MAX_SET_SIZE];
	
	// initialize to zero
	for (unsigned i = 0; i < N-1; i++) {
		for (unsigned j = i+1; j < N; j++) {
			edge_graphs[i][j] = 0;
			edge_weights[i][j] = 0;
		}
	}
	
// 	double t_start = get_time();
// 	int sample_checkpoint = 10000;
	
	for (int k = 0; k < n_samples; k++) {
// 		if (opt_output_sample_times && k == sample_checkpoint-1) {
// 			printf("Samples: %8i  time %f\n", sample_checkpoint, get_time() - t_start);
// 			sample_checkpoint *= 10;
// 		}
		
		// draw a sample
		TreeNode<Set> *root = sampler->sample();
		
		root->output();
		
		if (opt_output_edge_estimates) {
			double junction_trees = root->count_junction_trees();
			double partition_trees = junction_trees * root->nodes();
			double weight = 1.0 / partition_trees;
			weight_total += weight;
			
			Graph *G = root->graph();
			for (unsigned i = 0; i < N-1; i++) {
				for (unsigned j = i+1; j < N; j++) {
					if (G->has(i, j)) {
						edge_graphs[i][j] += 1;
						edge_weights[i][j] += weight;
					}
				}
			}
			delete G;
		}
		
		delete root;
	}
	
	if (opt_output_edge_estimates) {
		printf("total weight:  %f\n", weight_total);
		printf(" edge    graphs    weight         estimate\n");
		for (unsigned i = 0; i < N-1; i++) {
			for (unsigned j = i+1; j < N; j++) {
				double normalized = edge_weights[i][j] / weight_total;
				printf("%2i-%2i  %8i   %-14.6f  %f\n", i, j, edge_graphs[i][j], edge_weights[i][j], normalized);
			}
		}
	}
}



// args are [<number> [<seed>]]
void sampling(const char **argv)
{
	int n_samples = 1;
	rng.seed(time(NULL));
	srand(time(NULL));
	
	// number of samples
	if (*argv) n_samples = atoi(*argv++);
	
	// RNG seed
	if (*argv) {
		int seed = atoi(*argv);
		srand(seed);
		rng.seed(seed);
	}
	
// 	double t_start = get_time();
	
	allocate_tables();
	
	vbprintf("\nComputing sum tables...\n");
	double sum_score = compute_sum_f(Set::empty(N), Set::complete(N));
	vbprintf("Total score: %f\n", sum_score);
	
// 	double t_sums = get_time();
// 	if (opt_output_sample_times) printf("DP time:    %f\n", t_sums - t_start);
	
	Sampler *sampler;
	
	if (opt_naive_sampling) {
		sampler = new NaiveSampler();
	} else {
		sampler = new AdaptiveSampler();
	}
	
	sample(n_samples, sampler);
	delete sampler;
	
// 	double t_end = get_time();
// 	if (opt_output_sample_times) printf("Sampling time:   %f\n", t_end - t_sums);
	
	deallocate_tables();
}
