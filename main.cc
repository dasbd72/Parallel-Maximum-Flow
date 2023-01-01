#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ford-fulkerson.hh"
#include "graph.hh"
#include "parallel-push-relabel.hh"
#include "push-relabel.hh"
#include "utility.hh"

enum Method {
    ff,
    pr,
    ppr,
};
const Method method = ppr;

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
    switch (method) {
        case ff:
            TIMING_START(FordFulkerson);
            FordFulkerson(graph, flow);
            TIMING_END(FordFulkerson);
            break;
        case pr:
            TIMING_START(PushRelabel);
            PushRelabel(graph, flow);
            TIMING_END(PushRelabel);
            break;
        case ppr:
            TIMING_START(ParallelPushRelabel);
            ParallelPushRelabel(graph, flow);
            TIMING_END(ParallelPushRelabel);
            break;

        default:
            break;
    }
    // Max-Flow End

    // Output
    TIMING_START(Output);
    graph->output(flow);
    TIMING_END(Output);

    // Finalize
    delete graph;
    free(flow);
}