#include <cfloat>
#include <vector>
#include "set.hpp"
#include "graph.hpp"

int verbose = 1;
int output_solution = 1;
int output_tree = 1;
int output_matrix = 0;
int output_dot = 0;

#define vbprintf(...) if (verbose) fprintf (stdout, __VA_ARGS__)

unsigned N, W;
double *local_scores;

typedef SubsetArray<double> SetArray;

SetArray *h_values, *g_values, *f_values;
const double FLOAT_THRESHOLD = 0.000001;

template <typename Set>
double local_score(Set X)
{
	return local_scores[X.bits];
}


// Select a proper subset S of C to maximize f(S, R) / p(S)
template <typename Set>
double compute_h(Set C, Set R)
{
	double cached = h_values->get(C.bits, R.bits);
	if (cached != -DBL_MAX) return cached;
	
	double max_score = -DBL_MAX;
	
	// For each S in [Ø, C] not equal to C
	for (range_iterator<Set> it(N, Set::empty(N), C, 1, 0); it.has_next(); ++it) {
		Set S = it.set();
		
		double score_s = local_score(S);
		double score_f = compute_f(S, R);
		double score = score_f - score_s;
		
		if (score > max_score) max_score = score;
	}
	
	h_values->set(C.bits, R.bits, max_score);
	return max_score;
}


// Select non-empty R subset of U to maximize h(C, R) g(C, U \ R)
template <typename Set>
double compute_g(Set C, Set U)
{
	double cached = g_values->get(C.bits, U.bits);
	if (cached != -DBL_MAX) return cached;
	
	if (U.is_empty()) {
		h_values->set(C.bits, U.bits, 0.0);
		return 0.0;
	}
	
	double max_score = -DBL_MAX;
	
	unsigned f = U.first(N);	// the first element in U can always be placed in R
	
	// For each non-empty R in [Ø, U]
	for (range_iterator<Set> it(N, Set::empty(N) | f, U); it.has_next(); ++it) {
		Set R = it.set();
		
		double score_h = compute_h(C, R);
		double score_g = compute_g(C, U ^ R);
		double score = score_h + score_g;
		
		if (score > max_score) max_score = score;
	}
	
	g_values->set(C.bits, U.bits, max_score);
	return max_score;
}


// Select non-empty C in [S, S u R] with |C| < W to maximize p(C) g(C, R \ C)
template <typename Set>
double compute_f(Set S, Set R)
{
	// If h(C,R) is already cached, retrieve it.
	double cached = f_values->get(S.bits, R.bits);
	if (cached != -DBL_MAX) return cached;
	
	double max_score = -DBL_MAX;
	
	unsigned card_S = S.cardinality(N);
	assert((int)W - (int)card_S > 0);
	
	// If S is empty, by symmetry we can always put the first element of R in C
	Set from = card_S == 0 ? Set::empty(N) | R.first(N) : Set::empty(N);
	int start = card_S == 0 ? 1 : 0;
	
	// For each non-empty C in [S, S u R] with |C| <= W
	for (range_k_iterator<Set> it(N, W - card_S, from, R, start); it.has_next(); ++it) {
		Set D = it.set();
		Set C = S | D;		// 1 <= |D| <= W - |S|, thus 1 <= |C| <= W
		
		double score_c = local_score(C);
		double score_g = compute_g(C, R ^ D);
		double score = score_c + score_g;
		
		if (score > max_score) max_score = score;
	}
	
	f_values->set(S.bits, R.bits, max_score);
	return max_score;
}




// These are essentially the same functions as above, but instead of recursing on
// every possible choice, they backtrack on the choice that led to the optimal
// solution. Note that there might be multiple ways to construct an optimal junction
// tree, but the functions backtrack only on the first such path found.

template <typename Set>
void find_h(Set C, Set R, double score_m, TreeNode<Set> *node)
{
	for (range_iterator<Set> it(N, Set::empty(N), C, 1, 0); it.has_next(); ++it) {
		Set S = it.set();
		
		double score_s = local_score(S);
		double score_f = compute_f(S, R);
		double score = score_f - score_s;
		
// 		if (score == score_m) {
		if (fabs(score - score_m) <= FLOAT_THRESHOLD) {
			find_f(S, R, score_f, node);
			return;
		}
	}
	
	assert(0);
}


template <typename Set>
void find_g(Set C, Set U, double score_m, TreeNode<Set> *node)
{
	if (U.is_empty()) return;
	
	unsigned f = U.first(N);
	
	for (range_iterator<Set> it(N, Set::empty(N) | f, U); it.has_next(); ++it) {
		Set R = it.set();
		
		double score_h = compute_h(C, R);
		double score_g = compute_g(C, U ^ R);
		double score = score_h + score_g;
		
// 		if (score == score_m) {
		if (fabs(score - score_m) <= FLOAT_THRESHOLD) {
			find_h(C, R, score_h, node);
			find_g(C, U ^ R, score_g, node);
			return;
		}
	}
	
	assert(0);
}


template <typename Set>
TreeNode<Set> *find_f(Set S, Set R, double score_m, TreeNode<Set> *node)
{
	unsigned card_S = S.cardinality(N);
	
	Set from = card_S == 0 ? Set::empty(N) | R.first(N) : Set::empty(N);
	int start = card_S == 0 ? 1 : 0;
	
	for (range_k_iterator<Set> it(N, W - card_S, from, R, start); it.has_next(); ++it) {
		Set D = it.set();
		Set C = S | D;
		
		double score_c = local_score(C);
		double score_g = compute_g(C, R ^ D);
		double score = score_c + score_g;
		
// 		if (score == score_m) {
		if (fabs(score - score_m) <= FLOAT_THRESHOLD) {
			TreeNode<Set> *child = new TreeNode<Set>(N, C, local_score(C), S, local_score(S));
			if (node != NULL) node->add(child);
			find_g(C, R ^ D, score_g, child);
			return child;
		}
	}
	
	assert(0);
}




void line(const char *str)
{
	printf("====================================== %s\n", str);
}

template <typename Set>
void output(double max_score, TreeNode<Set> *root)
{
	if (output_solution) {
		line("Solution");
		printf("Cliques:\n");
		root->list_nodes();
		printf("\nSeparators:\n");
		root->list_separators();
		printf("\nTotal score of an optimal network:\n");
		printf("%16.6f\n", max_score);
	}
	
	if (output_tree) {
		line("Junction tree");
		root->print();
	}
	
	Graph *graph = root->graph();
	
	if (output_matrix) {
		line("Adjacency matrix");
		graph->print();
	}
	
	if (output_dot) {
		line(".dot");
		graph->graphviz(stdout);
	}
	
	delete graph;
}




template <typename Set>
void solve()
{
	// allocate memoization tables
	
	double required_memory = (double)SetArray::estimate(N, W) * 24 / 1024 / 1024;
	vbprintf("Estimated memory requirement: ");
	if (required_memory < 1000) {
		vbprintf("%.2f M\n", required_memory);
	} else {
		vbprintf("%.2f G\n", required_memory / 1024);
	}
	vbprintf("Allocating memoization tables f...");
	fflush(stdout);
	f_values = new SetArray(N, W);
	
	vbprintf(" g...");
	fflush(stdout);
	g_values = new SetArray(N, W);
	
	vbprintf(" h...");
	fflush(stdout);
	h_values = new SetArray(N, W);
	
	
	// dynamic programming
	
	vbprintf("\nSolving...\n");
	double max_score = compute_f(Set::empty(N), Set::complete(N));
	
	
	// backtrack to construct a junction tree
	
	vbprintf("Optimum found. Reconstructing...\n");
	TreeNode<Set> *root = find_f(Set::empty(N), Set::complete(N), max_score, (TreeNode<Set>*)NULL);
	
	
	// tables no longer needed, deallocate
	
	vbprintf("Network constructed. Deallocating...\n");
	delete [] local_scores;
	delete f_values;
	delete g_values;
	delete h_values;
	
	
	// output solution and other information
	
	output(max_score, root);
	delete root;
}


int read_flags(const char *flags)
{
	verbose = 0;
	output_solution = 0;
	output_tree = 0;
	output_matrix = 0;
	output_dot = 0;
	
	while (*flags != '\0') {
		char f = *flags;
		if (f == 'v') {
			verbose = 1;
		} else if (f == 's') {
			output_solution = 1;
		} else if (f == 't') {
			output_tree = 1;
		} else if (f == 'm') {
			output_matrix = 1;
		} else if (f == 'd') {
			output_dot = 1;
		} else if (f == 'a') {
			verbose = 1;
			output_solution = 1;
			output_tree = 1;
			output_matrix = 1;
			output_dot = 1;
		} else {
			printf("Error: Unknown flag: %c\n\n", f);
			return 0;
		}
		flags++;
	}
	
	return 1;
}

void print_usage(const char *cmd)
{
	printf("Usage: %s [<input file> [<maximum width>] [-flags]]\n", cmd);
	printf("\nFlags (default = -vst):\n");
	printf(" v:  verbose\n");
	printf(" s:  print the score of an optimal solution\n");
	printf(" t:  print a junction tree of the solution\n");
	printf(" m:  print the adjacency matrix of the solution\n");
	printf(" d:  print a .dot file of the solution\n");
	printf(" a:  set all flags\n");
	printf("\nExamples:\n\n");
	printf("%s bridges.score\n", cmd);
	printf("%s bridges.score 3\n", cmd);
	printf("%s flare.score -sm\n", cmd);
	printf("%s nursery.score 2 -vstm\n", cmd);
}



int main(int argc, char **argv)
{
	const char *cmd = argv[0];
	
	if (argc < 2) {
		print_usage(cmd);
		return 0;
	}
	
	const char *input_file = argv[1];
	const char *max_width = NULL;
	const char *flags = NULL;
	
	for (int i = 2; i < argc; i++) {
		if (argv[i][0] == '-') {
			flags = argv[i];
		} else {
			max_width = argv[i];
		}
	}
	
	if (flags != NULL && !read_flags(flags + 1)) {
		print_usage(cmd);
		return 0;
	}
	
	vbprintf("Input score file: %s\n", input_file);
	
	FILE *f = fopen(input_file, "r");
	if (f == NULL) {
		printf("Error: The input file could not be read.\n");
		return 0;
	}
	
	unsigned M;
	
	if (fscanf(f, "%u %u", &N, &M) != 2) {
		printf("Error: The input file is invalid. Could not read the number of variables and/or maximum set size.\n");
		return 0;
	}
	
	if (N > 32 || M > 32) {
		printf("Junctor can only handle instances of up to 32 variables.\n");
		return 0;
	}
	
	vbprintf("  Number of variables: %i\n", N);
	vbprintf("  Scores up to set size: %i\n", M);
	
	if (max_width != NULL) {
		W = atoi(max_width);
		if (W == 0) {
			printf("Error: The maximum width must be at least 1.\n");
			return 0;
		}
		if (W > M) {
			printf("Warning: Given maximum width was %i but the input only contains scores for sets up to size %i.\n", W, M);
			W = M;
		}
	} else {
		W = M;
	}
	
	vbprintf("Finding an optimal network of maximum width %i.\n", W);
	
	vbprintf("Reading input scores...\n");
	
	local_scores = new double[1 << N];
	
	set_iterator<uintset>::init(N);
	
	range_k_iterator<uintset> it(N, M, uintset::empty(N), uintset::complete(N));
	for (; it.has_next(); ++it) {
		if (fscanf(f, "%lf", &local_scores[it.set().bits]) != 1) {
			vbprintf("Error: The input file contains too few scores. No score for: ");
			it.set().rprintln(N);
			return 0;
		}
	}
	
	fclose(f);
	
	solve<uintset>();
}



