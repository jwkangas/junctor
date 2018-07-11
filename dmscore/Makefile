SHELL   = /bin/sh
CXX     = g++
#CXX     = clang++
PROGRAM = dmscore
FILES   = $(wildcard *.hpp)
CFLAGS  = -std=c++11 -Wall -Wextra -Werror -Werror=return-type -Wno-unused-parameter -Wno-unused-variable -Wno-comment -Wno-sign-compare
LDFLAGS = #-lboost_program_options

release: CFLAGS += -O2 -DNDEBUG 
release: $(PROGRAM)

debug: CFLAGS += -g
debug: $(PROGRAM)

profiled2: CFLAGS += -g -pg -O2 -DNDEBUG
profiled2: $(PROGRAM)

profiled1: CFLAGS += -g -pg -O1 -DNDEBUG
profiled1: $(PROGRAM)

$(PROGRAM): $(PROGRAM).cpp $(FILES)
	$(CXX) $(CFLAGS) $(PROGRAM).cpp $(LDFLAGS) -o $(PROGRAM)

clean:
	rm -f $(PROGRAM) gmon.out


