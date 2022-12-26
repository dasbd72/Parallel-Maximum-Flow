#include <omp.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <vector>

#include "graph.hh"
#include "utility.hh"

int getcf(int V, int S, int T, int *residual, int *rpath) {
    int cf = 0x7fffffff;
    for (int u = rpath[T], v = T; v != S; u = rpath[v = u]) {
        cf = std::min(cf, residual[u * V + v]);
    }
    return cf;
}

int getPath(int V, int S, int T, int *edge, int *nedge, int *residual, int *rpath, bool *visited) {
    std::queue<int> que;
    int u, v;

    memset(visited, false, sizeof(bool) * V);
    que.emplace(S);
    rpath[S] = S;
    visited[S] = true;

    while (!que.empty()) {
        u = que.front();
        que.pop();
        for (int i = 0; i < nedge[u]; i++) {
            v = edge[u * V + i];
            if (!visited[v] && residual[u * V + v] > 0) {
                rpath[v] = u;
                visited[v] = true;
                que.emplace(v);
                if (v == T) {
                    return getcf(V, S, T, residual, rpath);
                }
            }
        }
    }

    return 0;
}

void FordFulkerson(Graph *graph, int *flow) {
    int V = graph->V;
    int S = graph->S;
    int T = graph->T;
    int *capacity = (int *)malloc(V * V * sizeof(int));
    int *edge = (int *)malloc(V * V * sizeof(int));
    int *nedge = (int *)malloc(V * sizeof(int));
    int *residual = (int *)malloc(V * V * sizeof(int));
    int *rpath = (int *)malloc(V * sizeof(int));
    bool *visited = (bool *)malloc(V * sizeof(bool));
    int f, cf;

    memset(capacity, 0, V * V * sizeof(int));

    for (int r = 0; r < V; r++) {
        nedge[r] = graph->edge[r].size();
        for (int i = 0; i < nedge[r]; i++) {
            edge[r * V + i] = graph->edge[r][i].first;
            capacity[r * V + graph->edge[r][i].first] = graph->edge[r][i].second;
        }
    }
    for (int r = 0; r < V; r++) {
        for (int c = 0; c < V; c++) {
            residual[r * V + c] = capacity[r * V + c];
        }
    }

    for (f = 0; cf = getPath(V, S, T, edge, nedge, residual, rpath, visited); f += cf) {
        for (int u = rpath[T], v = T; v != S; u = rpath[v = u]) {
            residual[u * V + v] -= cf;
            residual[v * V + u] += cf;
        }
    }

    for (int r = 0; r < V; r++) {
        for (int c = 0; c < V; c++) {
            flow[r * V + c] = std::max(0, capacity[r * V + c] - residual[r * V + c]);
        }
    }

    free(capacity);
    free(edge);
    free(nedge);
    free(residual);
    free(rpath);
    free(visited);
}