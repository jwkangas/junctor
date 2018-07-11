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

#include <cstdlib>
#include <cmath>
// #include <sys/time.h>

#include "common.hpp"

Rng rng;



// output options
char output_flags[32];
int opt_verbose = 1;
int opt_output_headers = 1;
int opt_output_edge_estimates = 0;
int opt_naive_sampling = 0;
int opt_output_sample_times = 0;

// number of vertices, maximum width (clique size)
unsigned N, W;

// table of local components for each vertex subset
double *local_scores;

// max/sum arrays for dynamic programming over the space of RPTs
SetArray *f_values, *g_values, *h_values;




void allocate_tables()
{
	double required_memory = (double)SetArray::estimate(N, W) * 24 / 1024 / 1024;
	vbprintf("Estimated memory requirement: ");
	if (required_memory < 1000) {
		vbprintf("%.2f M\n", required_memory);
	} else {
		vbprintf("%.2f G\n", required_memory / 1024);
	}
	vbprintf("Allocating DP tables f...");
	fflush(stdout);
	f_values = new SetArray(N, W, -INFTY);
	
	vbprintf(" g...");
	fflush(stdout);
	g_values = new SetArray(N, W, -INFTY);
	
	vbprintf(" h...");
	fflush(stdout);
	h_values = new SetArray(N, W, -INFTY);
}

void deallocate_tables()
{
	vbprintf("Deallocating tables...\n");
	
	delete f_values;
	delete g_values;
	delete h_values;
}

// double get_time()
// {
// 	struct timeval tp;
// 	gettimeofday(&tp, NULL);
// 	double t = tp.tv_sec + tp.tv_usec / 1000000.0;
// 	return t;
// }





TreeNode<Set> *parse_tree(const char *&s, Set parent)
{
	Set C = Set(atoi(s));
	
	TreeNode<Set> *node = new TreeNode<Set>(C, C & parent);
	
	while (*s >= '0' && *s <= '9') s++;
	
	while (*s == '{') {
		s++;
		TreeNode<Set> *child = parse_tree(s, C);
		if (!child || *s != '}') {
			delete node;
			return NULL;
		}
		node->add(child);
		s++;
	}
	
	return node;
}

TreeNode<Set> *parse_tree(const char *s)
{
	return parse_tree(s, Set::empty(N));
}


