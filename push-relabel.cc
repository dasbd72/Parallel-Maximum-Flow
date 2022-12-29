#include <omp.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
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
    int *excess;
    int *residual;
    int *height;
};

void preflow(Data *data) {
    int V = data->V;
    int S = data->S;
    memset(data->excess, 0, sizeof(int) * V);
    memset(data->height, 0, sizeof(int) * V);
    data->height[S] = V;
    data->excess[S] = INT_MAX;
    for (int i = 0; i < data->nedge[S]; i++) {
        int v = data->edge[S * V + i];
        int delta = data->residual[S * V + v];
        if (delta) {
            data->residual[S * V + v] = 0;
            data->residual[v * V + S] += delta;
            data->excess[S] -= delta;
            data->excess[v] += delta;
        }
    }
}

// applies if excess[u] > 0, residual[u * V + v] > 0, and height[u] = height[v] + 1
void push(Data *data, int u, int v) {
    int V = data->V;
    int delta = min(data->excess[u], data->residual[u * V + v]);
    if (delta) {
        data->residual[u * V + v] -= delta;
        data->residual[v * V + u] += delta;
        data->excess[u] -= delta;
        data->excess[v] += delta;
    }
}

// applies if excess[u] > 0 and if height[u] < height[v] for all (u,v) residual[u * V + v] > 0
void relabel(Data *data, int u) {
    int V = data->V;
    int minHeight = INT_MAX;
    for (int v = 0; v < V; v++) {
        if (data->residual[u * V + v] > 0)
            minHeight = min(minHeight, data->height[v]);
    }
    data->height[u] = minHeight + 1;
}

int getExcess(Data *data) {
    int V = data->V;
    int S = data->S;
    int T = data->T;
    for (int i = 0; i < V; i++) {
        if (i != S && i != T && data->excess[i] > 0) {
            return i;
        }
    }
    return -1;
}

void PushRelabel(Graph *graph, int *flow) {
    Data *data = (Data *)malloc(sizeof(Data));
    int V = data->V = graph->V;
    int S = data->S = graph->S;
    int T = data->T = graph->T;
    int ncpus = data->ncpus = graph->ncpus;
    data->capacity = (int *)malloc(sizeof(int) * V * V);
    data->edge = (int *)malloc(sizeof(int) * V * V);
    data->nedge = (int *)malloc(sizeof(int) * V);
    data->excess = (int *)malloc(sizeof(int) * V);
    data->residual = (int *)malloc(sizeof(int) * V * V);
    data->height = (int *)malloc(sizeof(int) * V);

    memset(data->capacity, 0, sizeof(int) * V * V);
    for (int r = 0; r < V; r++) {
        data->nedge[r] = graph->edge[r].size();
        for (int i = 0; i < data->nedge[r]; i++) {
            data->edge[r * V + i] = graph->edge[r][i].first;
            data->capacity[r * V + graph->edge[r][i].first] = graph->edge[r][i].second;
        }
    }
    memcpy(data->residual, data->capacity, sizeof(int) * V * V);

    preflow(data);
    int u;
    while ((u = getExcess(data)) != -1) {
        bool isMin = true;
        for (int v = 0; v < V; v++) {
            if (data->residual[u * V + v] > 0) {
                if (data->height[u] > data->height[v]) {
                    push(data, u, v);
                    isMin = false;
                }
            }
        }
        if (isMin) {
            relabel(data, u);
        }
    }

    for (int r = 0; r < V; r++) {
        for (int i = 0; i < data->nedge[r]; i++) {
            int c = data->edge[r * V + i];
            flow[r * V + c] = data->capacity[r * V + c] - data->residual[r * V + c];
        }
    }

    free(data->capacity);
    free(data->edge);
    free(data->nedge);
    free(data->excess);
    free(data->residual);
    free(data->height);
    free(data);
}