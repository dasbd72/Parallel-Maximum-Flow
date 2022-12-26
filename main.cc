#include <cstdio>
#include <cstdlib>

#include "ford-fulkerson.hh"
#include "graph.hh"
#include "utility.hh"

int main(int argc, char **argv) {
    Graph *graph = new Graph(argc, argv);  // Graph
    int *flow;                             // Output flow matrix

    // Input
    TIMING_START(Input);
    graph->input();
    TIMING_END(Input);

    flow = (int *)malloc(graph->V * graph->V * sizeof(int));
    
    TIMING_START(FordFulkerson);
    FordFulkerson(graph, flow);
    TIMING_END(FordFulkerson);

    // Output
    TIMING_START(Output);
    graph->output(flow);
    TIMING_END(Output);

    // Finalize
    free(graph);
    free(flow);
}