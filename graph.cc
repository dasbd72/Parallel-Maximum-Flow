#include "graph.hh"

#include <omp.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

Graph::Graph(int argc, char **argv) {
    assert(argc == 5);
    input_filename = argv[1];
    output_filename = argv[2];
    S = atoi(argv[3]);
    T = atoi(argv[4]);
    ncpus = omp_get_max_threads();
}

void Graph::input() {
    FILE *input_file;
    int tmp[3];

    input_file = fopen(input_filename, "rb");
    fread(&V, sizeof(int), 1, input_file);
    fread(&E, sizeof(int), 1, input_file);
    edge.resize(V);
    for (int e = 0; e < E; e++) {
        fread(tmp, sizeof(int), 3, input_file);
        edge[tmp[0]].emplace_back(tmp[1], tmp[2]);
    }
    fclose(input_file);
}

void Graph::output(int *flow) {
    FILE *output_file;

    output_file = fopen(output_filename, "wb");
    fwrite(flow, sizeof(int), V * V, output_file);
    fclose(output_file);
}
