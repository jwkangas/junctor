/*
 *  DMScore
 *  
 *  Copyright 2015 Teppo Niinimäki <teppo.niinimaki(at)helsinki.fi>
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

#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <unistd.h>

#include <cassert>

#include "data.hpp"
#include "sortedsubset.hpp"
#include "subsets.hpp"
#include "subsetscore.hpp"
#include "boundedsubsetmap.hpp"


/**
 * Writes the scores to a stream.
 */
template <typename ScoreMap>
void writeScores(const ScoreMap& scores, const BoundedSubsets& subsets, std::string order, std::ostream& outStream) {
	int n = subsets.groundSet().size();
	if (order == "colex") {
		outStream << "colex_order" << " "
				<< subsets.maxSubsetSize() << "\n";
		for (auto& set : subsets.binaryAsc(SortedDownStackSubset(n))) {
			outStream << scores[set] << "\n";
		}
	}
	else if (order == "lex") {
		outStream << "lex_order" << " "
				<< subsets.maxSubsetSize() << "\n";
		for (auto& set : subsets.lexical(SortedStackSubset(n))) {
			outStream << scores[set] << "\n";
		}
	}
	else if (order == "free") {
		outStream << "free_order" << " "
				<< subsets.maxSubsetSize() << " "
				<< subsets.size() << "\n";
		for (auto& set : subsets.binaryAsc(SortedDownStackSubset(n))) {
			outStream << scores[set];
			outStream << " " << set.size();
			for (auto elem : set)
				outStream << " " << elem;
			outStream << "\n";
		}
	}
	else {
		assert(0);
		return;
	}
}



using namespace std;

/*
 * Main program.
 */
int main(int argc, char** argv) {
	
	bool printHelp = false;
	bool unifiedScores = true;
	string outFilename = "-";
	string setOutputOrder = "colex";
	
	// parse options
	char c;
	while ((c = getopt(argc, argv, "a:gho:")) != -1) {
		switch (c) {
			case 'a':
				setOutputOrder = optarg;
				if (setOutputOrder != "colex" &&
						setOutputOrder != "lex" &&
						setOutputOrder != "free") {
					fprintf(stderr, "Error: Invalid output order '%s'.\n",
							setOutputOrder.c_str());
					return 1;
				}
				break;
			case 'g':
				unifiedScores = false;
				break;
			case 'h':
				printHelp = true;
				break;
			case 'o':
				outFilename = optarg;
				break;
			case '?':
				return 1;
			default:
				assert(0);
				return 1;
		}
	}
	
	// print help if requested or wrong number of arguments
	if (printHelp || argc - optind < 2 || argc - optind > 3) {
		fprintf(stderr,
			"Syntax: %s [options] <datafile> <equivalent sample size>"
			" [<max clique size>]\n",
			argv[0]);
		return 1;
	}
	
	// parse input file name
	string inFilename = argv[optind];
	
	// parse ESS
	double equivalentSampleSize = atof(argv[optind + 1]);
	if (!(equivalentSampleSize > 0)) {
		fprintf(stderr, "Error: Invalid equivalent sample size.\n");
		return 1;
	}
	
	// parse max clique size
	int maxSetSize = - 1;
	if (argc - optind == 3) {
		maxSetSize = atoi(argv[optind + 2]);
	}
	
	// open the input stream
	istream inStream(0);
	ifstream inFile;
	if (inFilename == "-") {
		inStream.rdbuf(cin.rdbuf());
	} else {
		inFile.open(inFilename.c_str());
		if (!inFile) {
			fprintf(stderr, "Error: Could not open file '%s' for reading.\n",
					inFilename.c_str());
			return 1;
		}
		inStream.rdbuf(inFile.rdbuf());
	}

	// read the data
	CategoricalData<int> data;
	try {
		readData(inStream, &data);
	} catch (Exception& e) {
		fprintf(stderr, "Error: While reading data file '%s': %s\n",
				inFilename.c_str(), e.what());
		return 1;
	}
	if (inFile.is_open())
		inFile.close();

	int nVariables = data.getNumVariables();
	
	data.detectArities();
	
	// determine max clique size if not set, otherwise validate it
	if (maxSetSize == -1) {
		maxSetSize = nVariables;
	}	
	else if (!(1 <= maxSetSize && maxSetSize <= nVariables)) {
		fprintf(stderr, "Error: Invalid max clique size.\n");
		return 1;
	}

	// open the output stream
	ostream outStream(0);
	ofstream outFile;
	if (outFilename == "-") {
		outStream.rdbuf(cout.rdbuf());
	} else {
		outFile.open(outFilename.c_str());
		if (!outFile) {
			fprintf(stderr, "Error: Could not open file '%s' for writing.\n",
					outFilename.c_str());
			return 1;
		}
		outStream.rdbuf(outFile.rdbuf());
	}
	
	// create score function;
	std::unique_ptr<SubsetScore> scoreFun = nullptr;
	scoreFun = std::unique_ptr<SubsetScore>(new BDeuSubsetScore(equivalentSampleSize));
	
	// create a score map
	auto vars = ConstSortedSubset::fullSet(nVariables);
	BoundedSubsetMap<double> scores(vars, maxSetSize);
	
	// compute scores
	auto canGrow = [] (BoundedSubsetMap<double>::Subset s) { return s.canInsert(); };
	computeSubsetScores(&data, vars, canGrow, scoreFun.get(), scores);


	// output the header
	outStream << "DMST\n";
	outStream << nVariables << "\n";

	// output the scores
	outStream << std::fixed << std::setprecision(6);
	if (unifiedScores) {
		outStream << "subset_scores\n";
		auto subsets = boundedSubsets(vars, maxSetSize);
		writeScores(scores, subsets, setOutputOrder, outStream);
	}
	else {
		outStream << "clique_scores\n";
		auto cliques = boundedSubsets(vars, maxSetSize);
		writeScores(scores, cliques, setOutputOrder, outStream);
		outStream << "separator_scores\n";
		auto separators = boundedSubsets(vars, maxSetSize - 1);
		writeScores(scores, separators, setOutputOrder, outStream);
	}

	if (outFile.is_open())
		outFile.close();
	
	return 0;
}



