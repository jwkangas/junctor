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

double compute_sum_f(Set S, Set R);
double compute_sum_g(Set C, Set U);
double compute_sum_h(Set C, Set R);

TreeNode<Set> *sample_f_naive(Set S, Set R, TreeNode<Set> *node);
void sample_h_naive(Set C, Set R, TreeNode<Set> *node);
void sample_g_naive(Set C, Set U, TreeNode<Set> *node);




void sample_h_naive(Set C, Set R, TreeNode<Set> *node)
{
	double total = compute_sum_h(C, R);
	double P = log(rnd()) + total;
	
	double sum_score = -INFTY;
	
	H_ITERATE(it) {
		Set S = it.set();
		
		double score_s = local_score(S);
		double score_f = compute_sum_f(S, R);
		double score = score_f - score_s;
		
		sum_score = logsum(sum_score, score);
		
		if (sum_score >= P) {
			sample_f_naive(S, R, node);
			return;
		}
	}
	
	assert(0);
}


void sample_g_naive(Set C, Set U, TreeNode<Set> *node)
{
	if (U.is_empty()) return;
	
	double total = compute_sum_g(C, U);
	double P = log(rnd()) + total;
	
	double sum_score = -INFTY;
	
	G_ITERATE(it) {
		Set R = it.set();
		
		double score_h = compute_sum_h(C, R);
		double score_g = compute_sum_g(C, U ^ R);
		double score = score_h + score_g;
		
		sum_score = logsum(sum_score, score);
		
		if (sum_score >= P) {
			sample_h_naive(C, R, node);
			sample_g_naive(C, U ^ R, node);
			return;
		}
	}
	
	assert(0);
}


TreeNode<Set> *sample_f_naive(Set S, Set R, TreeNode<Set> *node)
{
	double total = compute_sum_f(S, R);
	double P = log(rnd()) + total;
	
	double sum_score = -INFTY;
	
	F_ITERATE(it) {
		Set D = it.set();
		Set C = S | D;
		
		double score_c = local_score(C);
		double score_g = compute_sum_g(C, R ^ D);
		double score = score_c + score_g;
		
		sum_score = logsum(sum_score, score);
		
		if (sum_score >= P) {
			TreeNode<Set> *child = new TreeNode<Set>(C, S);
			if (node != NULL) node->add(child);
			sample_g_naive(C, R ^ D, child);
			return child;
		}
	}
	
	assert(0);
}


TreeNode<Set> *sample_naive()
{
	return sample_f_naive(Set::empty(N), Set::complete(N), (TreeNode<Set>*)NULL);
}



