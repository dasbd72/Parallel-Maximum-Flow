#ifndef GRAPH
#define GRAPH
#include <utility>
#include <vector>

class Graph {
   public:
    char *input_filename;
    char *output_filename;
    int V;  // Number of vertices
    int E;  // Number of edges
    int S;
    int T;
    int ncpus;
    std::vector<std::vector<std::pair<int, int>>> edge;

    Graph(int argc, char **argv);
    ~Graph() {}
    void input();
    void output(int *flow);
};

#endif  // GRAPH