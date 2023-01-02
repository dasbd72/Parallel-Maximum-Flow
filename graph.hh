#ifndef GRAPH
#define GRAPH
#include <utility>
#include <vector>

class Graph {
   public:
    int V;  // Number of vertices
    int E;  // Number of edges
    int S;
    int T;
    double D;
    int ncpus;
    std::vector<std::vector<std::pair<int, int>>> edge;
    int *capacity;

    Graph(int argc, char **argv);
    ~Graph() {}
    void generate();
    void verify(int *flow);
};

#endif  // GRAPH