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

void find_global_optimum();
void sampling(const char **argv);


int read_flags(const char *flags)
{
	opt_verbose = 0;
	opt_output_headers = 0;
	opt_output_edge_estimates = 0;
	opt_naive_sampling = 0;
	opt_output_sample_times = 0;
	
	if (strlen(flags) > 16) {
		printf("Error: Too any input flags.\n");
		return 0;
	}
	
	strcpy(output_flags, flags);
	
	while (*flags != '\0') {
		char f = *flags;
		if (f == 'v') {
			opt_verbose = 1;
		} else if (f == 'h') {
			opt_output_headers = 1;
		} else if (f == 'e') {
			opt_output_edge_estimates = 1;
		} else if (f == 'n') {
			opt_naive_sampling = 1;
		} else if (f == 'T') {
			opt_output_sample_times = 1;
		} else if (!strchr("sjrtmdck", f)) {
			printf("Error: Unknown flag: %c\n\n", f);
			return 0;
		}
		flags++;
	}
	
	return 1;
}

void print_usage(const char *cmd)
{
	printf("Usage: %s [-flags] <input file> [<maximum width>] [<action [arg ...]>]\n", cmd);
	printf("\nAn action is one of: max, sample, tree, file, enum (default is max).\n");
	printf(" max                    find the maximum-a-posteriori graph\n");
	printf(" sample [<n> [<seed>]]  sample n junction trees with given RNG seed\n");
	printf(" tree <tree string>     parse the given tree in the compact form (-c)\n");
	printf(" file <tree file>       parse each tree in file in the compact form (-c)\n");
	printf(" enum                   enumerate all decomposable graphs, get edge probabilities\n");
	printf("\nFlags control what is printed for each resulting graph/tree:\n");
	printf(" s:  score\n");
	printf(" k:  cliques and separators\n");
	printf(" t:  tree representation\n");
	printf(" c:  compact tree representation (readable by adjunct)\n");
	printf(" j:  number of junction trees\n");
	printf(" r:  number of rooted junction trees (RPTs)\n");
	printf(" m:  adjacency matrix\n");
	printf(" d:  .dot file\n");
	printf("\nAdditional flags:\n");
	printf(" h:  print a header line before each output\n");
	printf(" v:  verbose, print information on computation progress\n");
	printf(" e:  in sampling, print estimates of edge probabilities\n");
	printf(" n:  use naive sampling (instead of adaptive)\n");
// 	printf(" T:  measure and print sampling time\n");
	printf("\nThe default flags are -ksthv\n");
	printf("\nExamples:\n");
	printf("\n%s bridges.score\n", cmd);
	printf("Find a maximum-a-posteriori graph for bridges.score.\n");
	printf("\n%s bridges.score 2 max\n", cmd);
	printf("Find a maximum-a-posteriori graph of maximum width 2.\n");
	printf("\n%s -the bridges.score sample 10\n", cmd);
	printf("Sample and print 10 junction trees and estimate edge probabilities.\n");
	printf("\n%s -s bridges.score tree 3{22}{513{1792{2304{2056{40}}{2176}}{320}}}\n", cmd);
	printf("Print the score of the input tree.\n");
}

// Sets N, W and local_scores values
int read_data(const char *input_file, const char *max_width)
{
	FILE *f = fopen(input_file, "r");
	if (f == NULL) {
		printf("Error: The input file could not be read.\n");
		return 1;
	}
	
	char filetype[5];
	if (fscanf(f, "%4s", filetype) != 1 || strcmp(filetype, "DMST")) {
		printf("Error: The input file is not of supported type.\n");
		return 1;
	}
	
	if (fscanf(f, "%u", &N) != 1) {
		printf("Error: Could not read the number of variables.\n");
		return 1;
	}
	
	vbprintf("  Number of variables: %u\n", N);
	
	char scoretype[65];
	if (fscanf(f, "%64s", scoretype) != 1 || strcmp(scoretype, "subset_scores")) {
		printf("Error: The input score type is not supported.\n");
		return 1;
	}
	
	char order[65];
	if (fscanf(f, "%64s", order) != 1 || strcmp(order, "colex_order")) {
		printf("Error: The score order is not supported.\n");
		return 1;
	}
	
	unsigned M;
	if (fscanf(f, "%u", &M) != 1) {
		printf("Error: Could not read the maximum set size.\n");
		return 1;
	}
	
	vbprintf("  Scores up to set size: %u\n", M);
	
	if (N > MAX_SET_SIZE || M > MAX_SET_SIZE) {
		printf("Error: Junctor can only handle instances of up to %i variables.\n", MAX_SET_SIZE);
		return 1;
	}
	
	if (max_width != NULL) {
		W = atoi(max_width);
		if (W == 0) {
			printf("Error: The maximum width must be at least 1.\n");
			return 1;
		}
		if (W > M) {
			printf("Warning: Given maximum width was %i but the input only contains scores for sets up to size %i.\n", W, M);
			W = M;
		}
	} else {
		W = M;
	}
	
	vbprintf("Reading input scores...\n");
	
	local_scores = new double[1 << N];
	
	range_k_iterator<Set>::init(N);
	
	for (range_k_iterator<Set> it(N, M, Set::empty(N), Set::complete(N)); it.has_next(); ++it) {
		if (fscanf(f, "%lf", &local_scores[it.set().bits]) != 1) {
			vbprintf("Error: The input file contains too few scores. No score for: ");
			it.set().rprintln(N);
			return 1;
		}
// 		printf("%f  ", local_scores[it.set().bits]); it.set().rprintln(N);
	}
	
	fclose(f);
	return 0;
}



void print_tree(const char *tree)
{
	TreeNode<Set> *root = parse_tree(tree);
	
	if (!root) {
		printf("Error: The tree string is malformed.\n");
		return;
	}
	
	root->output();
	delete root;
}

void input_tree(const char **argv)
{
	if (!*argv) {
		printf("Missing argument: A tree in the compact form.\n");
		return;
	}
	
	print_tree(*argv);
}

void input_tree_file(const char **argv)
{
	if (!*argv) {
		printf("Missing argument: A file containing trees in the compact form.\n");
		return;
	}
	
	FILE *f = fopen(*argv, "r");
	if (!f) {
		printf("Error: Could not read: %s\n", *argv);
		return;
	}
	
	char buffer[1024];
	
	while (fgets(buffer, 1024, f)) {
		*strchr(buffer, '\n') = '\0';
		print_tree(buffer);
	}
	
	fclose(f);
}

void enumerate()
{
	double probs[MAX_SET_SIZE * MAX_SET_SIZE];
	
	Graph *G = new Graph(N);
	G->local_scores = local_scores;
	G->enumerate_chordal(probs);
	
	for (unsigned i = 0; i < N-1; i++) {
		for (unsigned j = i+1; j < N; j++) {
			double p = probs[i*N+j];
			printf("%i-%i  %f\n", i, j, p);
		}
	}
	
	delete G;
}


#define END_USAGE { print_usage(cmd); return 0; }

int main(int, const char **argv)
{
	const char *cmd = *argv++;
	if (!*argv) END_USAGE;
	
	const char *arg = *argv;
	if (arg[0] == '-') {
		if (!read_flags(arg + 1)) END_USAGE;
		argv++;
		if (!*argv) END_USAGE;
	} else {
		strcpy(output_flags, "ksthv");
	}
	
	const char *input_file = *argv++;
	
	vbprintf("Input score file: %s\n", input_file);
	
	const char *max_width = NULL;
	if (*argv && atoi(*argv)) max_width = *argv++;
	
	if (read_data(input_file, max_width)) return 0;
	
	if (!*argv || !strcmp(*argv, "max")) {
		find_global_optimum();
	} else if (!strcmp(*argv, "sample")) {
		sampling(argv+1);
	} else if (!strcmp(*argv, "tree")) {
		input_tree(argv+1);
	} else if (!strcmp(*argv, "file")) {
		input_tree_file(argv+1);
	} else if (!strcmp(*argv, "enum")) {
		enumerate();
	} else {
		printf("Error: Unknown action.\n");
		print_usage(cmd);
	}
	
	delete [] local_scores;
}
