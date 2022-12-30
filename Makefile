CC = gcc
CXX = g++
LDFLAGS = -lm
CXXFLAGS = -Wall -Wextra -O3 -fopenmp
# CXXFLAGS += -g -fsanitize=address
CXXFLAGS += -DMETHOD=1
CXXFLAGS += -DTIMING
CXXFLAGS += -DDEBUG

EXE = main
OBJ = main.o graph.o utility.o ford-fulkerson.o push-relabel.o relabel-to-front.o

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

relabel-to-front.o: relabel-to-front.cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c $^

clean:
	rm -f $(EXE) $(OBJ)