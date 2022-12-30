#include "ford-fulkerson.hh"

#include <omp.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <vector>

#include "graph.hh"
#include "utility.hh"

struct Data {
    int V;
    int S;
    int T;
    int ncpus;
    int *capacity;
    int *edge;
    int *nedge;
    int *residual;
    int *rpath;
    bool *visited;
};

int getcf(Data *data) {
    int V = data->V;
    int S = data->S;
    int T = data->T;
    int cf = 0x7fffffff;
    for (int u = data->rpath[T], v = T; v != S; u = data->rpath[v = u]) {
        cf = min(cf, data->residual[u * V + v]);
    }
    return cf;
}

int getPath(Data *data) {
    int V = data->V;
    int S = data->S;
    int T = data->T;
    std::queue<int> que;
    int u, v;

    memset(data->visited, false, sizeof(bool) * V);
    que.emplace(S);
    data->rpath[S] = S;
    data->visited[S] = true;

    while (!que.empty()) {
        u = que.front();
        que.pop();
        for (int i = 0; i < data->nedge[u]; i++) {
            v = data->edge[u * V + i];
            if (!data->visited[v] && data->residual[u * V + v] > 0) {
                data->rpath[v] = u;
                data->visited[v] = true;
                que.emplace(v);
                if (v == T) {
                    return getcf(data);
                }
            }
        }
    }

    return 0;
}

void FordFulkerson(Graph *graph, int *flow) {
    Data *data = (Data *)malloc(sizeof(Data));
    int V = data->V = graph->V;
    int S = data->S = graph->S;
    int T = data->T = graph->T;
    data->capacity = (int *)malloc(V * V * sizeof(int));
    data->edge = (int *)malloc(V * V * sizeof(int));
    data->nedge = (int *)malloc(V * sizeof(int));
    data->residual = (int *)malloc(V * V * sizeof(int));
    data->rpath = (int *)malloc(V * sizeof(int));
    data->visited = (bool *)malloc(V * sizeof(bool));
    int f, cf;

    memset(data->capacity, 0, V * V * sizeof(int));

    for (int u = 0; u < V; u++) {
        data->nedge[u] = graph->edge[u].size();
        for (int i = 0; i < data->nedge[u]; i++) {
            int v = graph->edge[u][i].first;
            data->edge[u * V + i] = v;
            data->capacity[u * V + v] = graph->edge[u][i].second;
        }
    }
    for (int u = 0; u < V; u++) {
        for (int v = 0; v < V; v++) {
            data->residual[u * V + v] = data->capacity[u * V + v];
        }
    }

    for (f = 0; (cf = getPath(data)); f += cf) {
        for (int u = data->rpath[T], v = T; v != S; u = data->rpath[v = u]) {
            data->residual[u * V + v] -= cf;
            data->residual[v * V + u] += cf;
        }
    }

    for (int u = 0; u < V; u++) {
        for (int i = 0; i < data->nedge[u]; i++) {
            int v = data->edge[u * V + i];
            flow[u * V + v] = data->capacity[u * V + v] - data->residual[u * V + v];
        }
    }

    free(data->capacity);
    free(data->edge);
    free(data->nedge);
    free(data->residual);
    free(data->rpath);
    free(data->visited);
    free(data);
}