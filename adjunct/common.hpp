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

#ifndef COMMON_H
#define COMMON_H

#include <random>

#include "tools.hpp"
#include "set.hpp"
#include "graph.hpp"

typedef std::mt19937 Rng;
extern Rng rng;

typedef uintset Set;
typedef DisjointPairArray<double> SetArray;

extern unsigned N, W;
extern double *local_scores;
extern SetArray *f_values, *g_values, *h_values;

extern char output_flags[32];
extern int opt_verbose;
extern int opt_output_headers;
extern int opt_output_edge_estimates;
extern int opt_naive_sampling;
extern int opt_output_sample_times;


#define vbprintf(...) if (opt_verbose) fprintf (stdout, __VA_ARGS__)
#define local_score(X) (local_scores[X.bits])

void allocate_tables();
void deallocate_tables();

// double get_time();


#define H_ITERATE(it) \
for (range_iterator<Set> it(N, Set::empty(N), C, 1, 0); it.has_next(); ++it)

#define G_ITERATE(it) \
unsigned f = U.first(N); \
for (range_iterator<Set> it(N, Set::empty(N) | f, U); it.has_next(); ++it)

#define F_ITERATE(it) \
unsigned card_S = S.cardinality(N); \
assert((int)W - (int)card_S > 0); \
for (range_k_iterator<Set> it(N, W - card_S, Set::empty(N), R, 0); it.has_next(); ++it) 

// In maximation only, if the separator S is empty, then we can always
// put the lexicografically least element of R into C.
// This is still guaranteed to consider at least one optimal solution.
#define F_ITERATE_OPT(it) \
unsigned card_S = S.cardinality(N); \
assert((int)W - (int)card_S > 0); \
Set from = card_S == 0 ? Set::empty(N) | R.first(N) : Set::empty(N); \
int start = card_S == 0 ? 1 : 0; \
for (range_k_iterator<Set> it(N, W - card_S, from, R, start); it.has_next(); ++it)


template <typename Set>
struct TreeNode
{
	std::vector<TreeNode*> children;
	Set C, S;
	
	TreeNode(Set C, Set S) : children(), C(C), S(S) {}
	
	~TreeNode()
	{
		for (unsigned i = 0; i < children.size(); i++) {
			delete children[i];
		}
	}
	
	void add(TreeNode *child)
	{
		children.push_back(child);
	}
	
	void spaces(int n)
	{
		for (int i = 0; i < n; i++) printf(" ");
	}
	
	int maxspace(int d, int w)
	{
		return 3 * d + 3 * w + 1;
	}
	
	void print(int d, int w, int level, int *bars)
	{
		char buffer[128] = "";
		
		for (int i = 0; i < level; i++) {
			if (i == level - 1) {
				strcat(buffer, bars[i] ? "+--" : "+--");
			} else {
				strcat(buffer, bars[i] ? "|  " : "   ");
			}
		}
		
		C.rcat(buffer, N);
		
		printf("%s", buffer);
		
		if (!S.is_empty()) {
			spaces(maxspace(d, w) - strlen(buffer));
			S.rprintln(N);
		} else {
			printf("\n");
		}
		
		for (unsigned i = 0; i < children.size(); i++) {
			bars[level] = i < children.size() - 1;
			children[i]->print(d, w, level + 1, bars);
		}
	}
	
	// returns the width (maximum clique size) of the tree
	int width()
	{
		int w = C.cardinality(N);
		
		for (unsigned i = 0; i < children.size(); i++) {
			int subtree_width = children[i]->width();
			if (subtree_width > w) w = subtree_width;
		}
		
		return w;
	}
	
	// returns the depth of the tree
	int depth()
	{
		int d = 0;
		
		for (unsigned i = 0; i < children.size(); i++) {
			int cd = children[i]->depth() + 1;
			if (cd > d) d = cd;
		}
		
		return d;
	}
	
	// returns the number of nodes (cliques) in the tree
	int nodes()
	{
		int n = 1;
		
		for (unsigned i = 0; i < children.size(); i++) {
			n += children[i]->nodes();
		}
		
		return n;
	}
	
	// returns the score of the tree with respect to the local_scores table,
	// i.e., the scores of the cliques minus the scores of the separators
	double score()
	{
		double total = local_score(C) - local_score(S);
		for (unsigned i = 0; i < children.size(); i++) {
			total += children[i]->score();
		}
		return total;
	}
	
	void print()
	{
		int bars[MAX_SET_SIZE];
		for (unsigned i = 0; i < N; i++) bars[i] = 0;
		
		int d = depth();
		int w = width();
		
		print(d, w, 0, bars);
	}
	
	void makegraph(Graph *graph)
	{
		int n = graph->n;
		int elements[MAX_SET_SIZE];
		int k = C.get_list(n, elements);
		
		for (int i = 0; i < k-1; i++) {
			for (int j = i+1; j < k; j++) {
				graph->add(elements[i], elements[j]);
			}
		}
		
		for (unsigned i = 0; i < children.size(); i++) {
			children[i]->makegraph(graph);
		}
	}
	
	Graph* graph()
	{
		Graph* graph = new Graph(N);
		makegraph(graph);
		return graph;
	}
	
	void list_nodes()
	{
		printf("%16.6f  ", local_score(C));
		C.rprintln(N);
		
		for (unsigned i = 0; i < children.size(); i++) {
			children[i]->list_nodes();
		}
	}
	
	void list_separators()
	{
		for (unsigned i = 0; i < children.size(); i++) {
			Set S = children[i]->S;
			printf("%16.6f  ", local_score(S));
			S.rprintln(N);
			children[i]->list_separators();
		}
	}
	
	
	// Finds all unique intersections (separators without duplicates)
	// and stores them in intersections, k counting their number so far
	void find_intersections(Set *intersections, int &k)
	{
		for (unsigned i = 0; i < children.size(); i++) {
			Set S = children[i]->S;
			
			int j;
			for (j = 0; j < k; j++) {
				if (intersections[j] == S) break;
			}
			
			// if intersection not already in list, add it
			if (j == k) intersections[k++] = S;
			
			// recurse on children
			children[i]->find_intersections(intersections, k);
		}
	}
	
	int find_intersection_subtree(Set I, int &n_nodes, int &n_components, double &product)
	{
		if (!I.subsetof(C)) return 0;
		
		n_nodes++;
		
		int nodes = 1;
		
		for (unsigned i = 0; i < children.size(); i++) {
			int subtree_nodes = children[i]->find_intersection_subtree(I, n_nodes, n_components, product);
			if (children[i]->S == I) {
				n_components++;
				product *= subtree_nodes;
			} else {
				nodes += subtree_nodes;
			}
		}
		
		return nodes;
	}
	
	double find_intersection_root(Set I)
	{
		if (I.subsetof(C)) {
			int n_nodes = 0;
			int n_components = 1;
			double product = 1;
			int nodes = find_intersection_subtree(I, n_nodes, n_components, product);
			product *= nodes;
			
			return pow(n_nodes, n_components - 2) * product;
		}
		
		for (unsigned i = 0; i < children.size(); i++) {
			double trees = children[i]->find_intersection_root(I);
			if (trees > 0) return trees;
		}
		
		return 0;
	}
	
	// counts the number of clique trees that represent the same chordal graph,
	// uses essentially the algorithm in Thomas & Green '09 (JCGS)
	double count_junction_trees()
	{
		Set intersections[MAX_SET_SIZE];
		int k = 0;
		
		find_intersections(intersections, k);
		
		double junction_trees = 1;
		
		for (int i = 0; i < k; i++) {
			junction_trees *= find_intersection_root(intersections[i]);
		}
		
		return junction_trees;
	}
	
	void serialize_ref(char *&s)
	{
		sprintf(s, "%i", C.bits);
		s = strchr(s, '\0');
		for (unsigned i = 0; i < children.size(); i++) {
			*s++ = '{';
			children[i]->serialize_ref(s);
			*s++ = '}';
		}
	}
	
	void serialize(char *s)
	{
		serialize_ref(s);
		*s = '\0';
	}
	
	void header(const char *str)
	{
		if (!opt_output_headers) return;
		
		printf("====================================== %s\n", str);
	}
	
	void output()
	{
		double junction_trees = -1;
		Graph *gr = NULL;
		
		for (const char *p = output_flags; *p != '\0'; p++) {
			if (*p == 's') {
				header("Score");
				printf("%f\n", score());
			} else if (*p == 'c') {
				header("Compact");
				char s[1024];
				serialize(s);
				printf("%s\n", s);
			} else if (*p == 'j') {
				header("Junction trees");
				if (junction_trees == -1) junction_trees = count_junction_trees();
				printf("%f\n", junction_trees);
			} else if (*p == 'r') {
				header("Rooted junction trees");
				if (junction_trees == -1) junction_trees = count_junction_trees();
				printf("%f\n", junction_trees * nodes());
			} else if (*p == 't') {
				header("Tree");
				print();
			} else if (*p == 'k') {
				header("Cliques and separators");
				printf("Cliques:\n");
				list_nodes();
				printf("Separators:\n");
				list_separators();
			} else if (*p == 'm') {
				header("Adjacency matrix");
				if (gr == NULL) gr = graph();
				gr->print();
			} else if (*p == 'd') {
				header(".dot");
				if (gr == NULL) gr = graph();
				gr->make_dot(stdout);
			}
		}
		
		if (gr) delete gr;
	}
};





TreeNode<Set> *parse_tree(const char *s);




#endif
