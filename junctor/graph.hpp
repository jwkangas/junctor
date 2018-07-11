#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <cfloat>
#include "set.hpp"



struct Graph
{
	int edges[32][32];
	int n;
	
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
		edges[i][j] = 1;
		edges[j][i] = 1;
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
	
	void graphviz(FILE *f)
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
};



template <typename Set>
struct TreeNode
{
	std::vector<TreeNode*> children;
	int N;
	Set C;
	double C_score;
	Set S;
	double S_score;
	
	TreeNode(int N, Set C, double C_score, Set S, double S_score) :
		children(),
		N(N),
		C(C),
		C_score(C_score),
		S(S),
		S_score(S_score)
		{}
	
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
		for (unsigned i = 0; i < n; i++) printf(" ");
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
	
	int width()
	{
		int w = C.cardinality(N);
		
		for (unsigned i = 0; i < children.size(); i++) {
			int subtree_width = children[i]->width();
			if (subtree_width > w) w = subtree_width;
		}
		
		return w;
	}
	
	int depth()
	{
		int d = 0;
		
		for (unsigned i = 0; i < children.size(); i++) {
			int a = children[i]->depth() + 1;
			if (a > d) d = a;
		}
		
		return d;
	}
	
	void print()
	{
		int bars[N];
		for (int i = 0; i < N; i++) bars[i] = 0;
		
		int d = depth();
		int w = width();
// 		printf("Cliques ");
// 		spaces(maxspace(d, w) - 11);
// 		printf("Separators\n");
		
		print(d, w, 0, bars);
		
// 		printf("w: %i d %i\n", d, w);
	}
	
	void makegraph(Graph *graph)
	{
		int n = graph->n;
		int elements[n];
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
		printf("%16.6f  ", C_score);
		C.rprintln(N);
		
		for (unsigned i = 0; i < children.size(); i++) {
			children[i]->list_nodes();
		}
	}
	
	void list_separators()
	{
		if (!S.is_empty()) {
			printf("%16.6f  ", -S_score);
			S.rprintln(N);
		}
		
		for (unsigned i = 0; i < children.size(); i++) {
			children[i]->list_separators();
		}
	}
};





#endif
