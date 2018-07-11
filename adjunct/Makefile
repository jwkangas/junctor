CXX = g++
FLAGS = -std=c++11 -O3 -pedantic -Wall -Wextra

adjunct: common.o adjunct.o maximization.o sampling.o sampling_adaptive.o sampling_naive.o tools.o
	$(CXX) $(FLAGS) -o adjunct common.o adjunct.o maximization.o sampling.o sampling_adaptive.o sampling_naive.o tools.o

common.o: common.cpp common.hpp tools.hpp set.hpp graph.hpp
	$(CXX) $(FLAGS) -c common.cpp

adjunct.o: adjunct.cpp common.hpp tools.hpp set.hpp graph.hpp
	$(CXX) $(FLAGS) -c adjunct.cpp

maximization.o: maximization.cpp common.hpp tools.hpp set.hpp graph.hpp
	$(CXX) $(FLAGS) -c maximization.cpp

sampling.o: sampling.cpp common.hpp discretedist.hpp tools.hpp set.hpp graph.hpp
	$(CXX) $(FLAGS) -c sampling.cpp

sampling_adaptive.o: sampling_adaptive.cpp common.hpp discretedist.hpp tools.hpp set.hpp graph.hpp
	$(CXX) $(FLAGS) -c sampling_adaptive.cpp

sampling_naive.o: sampling_naive.cpp common.hpp tools.hpp set.hpp graph.hpp
	$(CXX) $(FLAGS) -c sampling_naive.cpp

tools.o: tools.cpp tools.hpp
	$(CXX) $(FLAGS) -c tools.cpp
