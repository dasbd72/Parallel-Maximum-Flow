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
const Method method = METHOD;

int main(int argc, char **argv) {
    Graph *graph = new Graph(argc, argv);  // Graph
    int *flow;                             // Output flow matrix

    // Generate
    TIMING_START(Generate);
    graph->generate();
    TIMING_END(Generate);

    flow = (int *)malloc(graph->V * graph->V * sizeof(int));
    memset(flow, 0, graph->V * graph->V * sizeof(int));

    printf("V: %d\n", graph->V);
    printf("E: %d\n", graph->E);

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

    // Verify
    TIMING_START(Verify);
    graph->verify(flow);
    TIMING_END(Verify);

    // Finalize
    delete graph;
    free(flow);
}