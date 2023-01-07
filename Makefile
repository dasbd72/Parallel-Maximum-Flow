CC = gcc
CXX = g++
LDFLAGS = -lm
CXXFLAGS = -fopenmp -pthread
CXXFLAGS += -O3
CXXFLAGS += -Wall -Wextra
# CXXFLAGS += -g -fsanitize=address
CXXFLAGS += -DTIMING
CXXFLAGS += -DDEBUG
CXXFLAGS += -DGRAPH_ONE_WAY
CXXFLAGS += -DGRAPH_ACYCLIC
CXXFLAGS += -DSPINLOCK
CXXFLAGS += -DMETHOD=ppr
CXXFLAGS += -DQTYPE=0

EXE = main
OBJ = main.o graph.o utility.o ford-fulkerson.o push-relabel.o parallel-push-relabel.o

alls: $(EXE)

main: $(OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

main.o: main.cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c $^

graph.o: graph.cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c $^

utility.o: utility.cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c $^

ford-fulkerson.o: ford-fulkerson.cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c $^

push-relabel.o: push-relabel.cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c $^

parallel-push-relabel.o: parallel-push-relabel.cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c $^

clean:
	rm -f $(EXE) $(OBJ)