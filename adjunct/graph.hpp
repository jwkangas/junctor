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

#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <cfloat>
#include <limits>
#include "set.hpp"
#include "tools.hpp"


struct list
{
	int *vertices;
	int n;
	
	list(int n) : vertices(new int[n]), n(0) {}
	
	void free()
	{
		delete [] vertices;
	}
	
	~list()
	{
		delete [] vertices;
	}
	
	void add(int u)
	{
		vertices[n] = u;
		n++;
	}
	
	int operator [] (int i)
	{
		return vertices[i];
	}
	
	int item(int i)
	{
		return vertices[i];
	}
	
	void print()
	{
		for (int i = 0; i < n; i++) {
			printf("%i ", vertices[i]);
		}
		printf("\n");
	}
	
	unsigned get_int()
	{
		unsigned s = 0;
		for (int i = 0; i < n; i++) s += 1 << item(i);
		return s;
	}
};



struct Graph
{
	bool edges[MAX_SET_SIZE][MAX_SET_SIZE];
	int n;
	double *local_scores;
	double edge_p[MAX_SET_SIZE][MAX_SET_SIZE];
	int n_chordal;
	double score_total;
	
	Graph(int n) : n(n)
	{
		for (int i = 0; i < n; i++) {
			for (int j = 0; j < n; j++) {
				edges[i][j] = 0;
			}
		}
	}
	
	void add(int i, int j)
	{
		edges[i][j] = true;
		edges[j][i] = true;
	}
	
	void del(int i, int j)
	{
		edges[i][j] = false;
		edges[j][i] = false;
	}
	
	bool has(int i, int j)
	{
		return edges[i][j];
	}
	
	void print()
	{
		for (int j = 0; j < n; j++) {
			for (int i = 0; i < n; i++) {
				printf("%i", edges[i][j]);
			}
			printf("\n");
		}
	}
	
	void print_edges()
	{
		for (int i = 0; i < n-1; i++) {
			for (int j = i+1; j < n; j++) {
				if (has(i, j)) printf("%i-%i ", i, j);
			}
		}
		printf("\n");
	}
	
	// returns the neighbor set of u in subset
	list *neighbors(int u, bool *subset)
	{
		list *set = new list(n);
		
		for (int i = 0; i < n; i++) {
			if (!subset[i]) continue;
			if (has(u, i)) set->add(i);
		}
		
		return set;
	}
	
	// returns true iff every pair vertices in set is adjacent to each other
	bool is_clique(list *set)
	{
		for (int i = 0; i < set->n-1; i++) {
			int u = set->item(i);
			for (int j = i+1; j < set->n; j++) {
				if (!has(u, set->item(j))) return false;
			}
		}
		
		return true;
	}
	
	// returns true iff vertex u is adjacent to all vertices in set
	bool is_adjacent_to_all(int u, list *set)
	{
		for (int i = 0; i < set->n; i++) {
			if (!has(u, set->item(i))) return false;
		}
		
		return true;
	}
	
	// returns true iff list of k vertices is adjacent to a vertex in subset
	bool has_common_neighbor(list *set, bool *subset)
	{
		for (int i = 0; i < n; i++) {
			if (!subset[i]) continue;
			if (is_adjacent_to_all(i, set)) return true;
		}
		
		return false;
	}
	
	// returns true iff list of k vertices is adjacent to a vertex
	bool has_common_neighbor(list *set)
	{
		for (int i = 0; i < n; i++) {
			if (is_adjacent_to_all(i, set)) return true;
		}
		
		return false;
	}
	
	// returns true iff all neighbors of u in subset form a clique
	bool is_simplicial(int u, bool *subset)
	{
		list *nset = neighbors(u, subset);
		bool is = is_clique(nset);
		delete nset;
		return is;
	}
	
	// returns a simplicial vertex in subset or -1 if no such vertex
	int find_simplicial(bool *subset)
	{
		for (int i = 0; i < n; i++) {
			if (!subset[i]) continue;
			if (is_simplicial(i, subset)) return i;
		}
		
		return -1;
	}
	
	void make_dot(FILE *f)
	{
		fprintf(f, "graph G {\n");
		
		for (int i = 0; i < n; i++) fprintf(f, "\t%i;\n", i);
		
		for (int j = 0; j < n-1; j++) {
			for (int i = j+1; i < n; i++) {
				if (!edges[i][j]) continue;
				fprintf(f, "\t%i -- %i;\n", i, j);
			}
		}
		
		fprintf(f, "}\n");
	}
	
	// determines the cliques and separators and computes the total score
	double get_score(int &n_cliques)
	{
		double score = 0.0;
		n_cliques = 0;
		bool subset[MAX_SET_SIZE];
		for (int i = 0; i < n; i++) subset[i] = true;
		
		for (int i = 0; i < n; i++) {
			int s = find_simplicial(subset);
			if (s == -1) return DBL_MAX;
			
			subset[s] = false;
			list *potential = neighbors(s, subset);
			
			if (has_common_neighbor(potential, subset)) {
// 				printf("S: "); potential->print();
				score -= local_scores[potential->get_int()];
			}
			
			potential->add(s);
			
			if (!has_common_neighbor(potential)) {
// 				printf("C: "); potential->print();
				score += local_scores[potential->get_int()];
				n_cliques++;
			}
			
			delete potential;
		}
		
		return score;
	}
	
	
	void enum_chordal_check()
	{
		int n_cliques;
		
		double score = get_score(n_cliques);
		if (score == DBL_MAX) return;
		
		n_chordal++;
		score_total = logsum(score_total, score);
		
		for (int i = 0; i < n-1; i++) {
			for (int j = i+1; j < n; j++) {
				if (!has(i, j)) continue;
				edge_p[i][j] = logsum(edge_p[i][j], score);
			}
		}
		
		if (n_chordal % 1000000 == 0) {
			printf("%i\n", n_chordal);
		}
	}
	
	void enum_chordal_branch(int i, int j)
	{
		if (i == n) {
			enum_chordal_check();
			return;
		}
		
		if (j == n) {
			enum_chordal_branch(i+1, i+2);
			return;
		}
		
		enum_chordal_branch(i, j+1);
		add(i, j);
		enum_chordal_branch(i, j+1);
		del(i, j);
	}
	
	void enumerate_chordal(double *probs)
	{
		n_chordal = 0;
		score_total = -std::numeric_limits<double>::infinity();
		
		for (int i = 0; i < n-1; i++) {
			for (int j = i+1; j < n; j++) {
				edge_p[i][j] = -std::numeric_limits<double>::infinity();
			}
		}
		
		printf("Enumerating all decomposable graphs...\n");
		
		enum_chordal_branch(0, 1);
		
		printf("Networks:     %i\n", n_chordal);
		printf("Total score:  %f\n", score_total);
		printf("Edge probabilities:\n");
		
		for (int i = 0; i < n-1; i++) {
			for (int j = i+1; j < n; j++) {
				probs[i*n+j] = exp(edge_p[i][j] - score_total);
			}
		}
	}
};



#endif
