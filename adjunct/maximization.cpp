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


#define FLOAT_EQUALS(a, b) (fabs((a) - (b)) <= 0.000001)
// #define FLOAT_EQUALS(a, b) ((a) == (b))


double compute_max_f(Set S, Set R);
double compute_max_g(Set C, Set U);
double compute_max_h(Set C, Set R);

TreeNode<Set> *backtrack_max_f(Set S, Set R, double score_m, TreeNode<Set> *node);
void backtrack_max_g(Set C, Set U, double score_m, TreeNode<Set> *node);
void backtrack_max_h(Set C, Set R, double score_m, TreeNode<Set> *node);



double compute_max_h(Set C, Set R)
{
	double cached = h_values->get(C.bits, R.bits);
	if (cached != -INFTY) return cached;
	
	double max_score = -INFTY;
	
	H_ITERATE(it) {
		Set S = it.set();
		
		double score_s = local_score(S);
		double score_f = compute_max_f(S, R);
		double score = score_f - score_s;
		
		if (score > max_score) max_score = score;
	}
	
	h_values->set(C.bits, R.bits, max_score);
	return max_score;
}


double compute_max_g(Set C, Set U)
{
	double cached = g_values->get(C.bits, U.bits);
	if (cached != -INFTY) return cached;
	
	if (U.is_empty()) {
		g_values->set(C.bits, U.bits, 0.0);
		return 0.0;
	}
	
	double max_score = -INFTY;
	
	G_ITERATE(it) {
		Set R = it.set();
		
		double score_h = compute_max_h(C, R);
		double score_g = compute_max_g(C, U ^ R);
		double score = score_h + score_g;
		
		if (score > max_score) max_score = score;
	}
	
	g_values->set(C.bits, U.bits, max_score);
	return max_score;
}


double compute_max_f(Set S, Set R)
{
	double cached = f_values->get(S.bits, R.bits);
	if (cached != -INFTY) return cached;
	
	double max_score = -INFTY;
	
	F_ITERATE_OPT(it) {
		Set D = it.set();
		Set C = S | D;
		
		double score_c = local_score(C);
		double score_g = compute_max_g(C, R ^ D);
		double score = score_c + score_g;
		
		if (score > max_score) max_score = score;
	}
	
	f_values->set(S.bits, R.bits, max_score);
	return max_score;
}




void backtrack_max_h(Set C, Set R, double score_m, TreeNode<Set> *node)
{
	H_ITERATE(it) {
		Set S = it.set();
		
		double score_s = local_score(S);
		double score_f = compute_max_f(S, R);
		double score = score_f - score_s;
		
		if FLOAT_EQUALS(score, score_m) {
			backtrack_max_f(S, R, score_f, node);
			return;
		}
	}
	
	assert(0);
}


void backtrack_max_g(Set C, Set U, double score_m, TreeNode<Set> *node)
{
	if (U.is_empty()) return;
	
	G_ITERATE(it) {
		Set R = it.set();
		
		double score_h = compute_max_h(C, R);
		double score_g = compute_max_g(C, U ^ R);
		double score = score_h + score_g;
		
		if FLOAT_EQUALS(score, score_m) {
			backtrack_max_h(C, R, score_h, node);
			backtrack_max_g(C, U ^ R, score_g, node);
			return;
		}
	}
	
	assert(0);
}


TreeNode<Set> *backtrack_max_f(Set S, Set R, double score_m, TreeNode<Set> *node)
{
	F_ITERATE_OPT(it) {
		Set D = it.set();
		Set C = S | D;
		
		double score_c = local_score(C);
		double score_g = compute_max_g(C, R ^ D);
		double score = score_c + score_g;
		
		if FLOAT_EQUALS(score, score_m) {
			TreeNode<Set> *child = new TreeNode<Set>(C, S);
			if (node != NULL) node->add(child);
			backtrack_max_g(C, R ^ D, score_g, child);
			return child;
		}
	}
	
	assert(0);
}



void find_global_optimum()
{
	allocate_tables();
	
	vbprintf("\nComputing max tables...\n");
	double max_score = compute_max_f(Set::empty(N), Set::complete(N));
	
	vbprintf("Optimum found. Backtracking...\n");
	TreeNode<Set> *root = backtrack_max_f(Set::empty(N), Set::complete(N), max_score, (TreeNode<Set>*)NULL);
	
	root->output();
	
	deallocate_tables();
	
	delete root;
}


