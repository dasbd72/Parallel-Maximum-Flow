#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ford-fulkerson.hh"
#include "graph.hh"
#include "push-relabel.hh"
#include "utility.hh"

#ifndef METHOD
#define METHOD 1
#endif

int main(int argc, char **argv) {
    Graph *graph = new Graph(argc, argv);  // Graph
    int *flow;                             // Output flow matrix

    // Input
    TIMING_START(Input);
    graph->input();
    TIMING_END(Input);

    flow = (int *)malloc(graph->V * graph->V * sizeof(int));
    memset(flow, 0, graph->V * graph->V * sizeof(int));

    // Max-Flow
#if METHOD == 0
    TIMING_START(FordFulkerson);
    FordFulkerson(graph, flow);
    TIMING_END(FordFulkerson);
#elif METHOD == 1
    TIMING_START(PushRelabel);
    PushRelabel(graph, flow);
    TIMING_END(PushRelabel);
#endif

    // Max-Flow End

    // Output
    TIMING_START(Output);
    graph->output(flow);
    TIMING_END(Output);

    // Finalize
    delete graph;
    free(flow);
}