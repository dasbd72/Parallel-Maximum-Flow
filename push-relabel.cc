#include "push-relabel.hh"

#include <omp.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "graph.hh"
#include "utility.hh"

namespace PR {
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
    int fifoHead;
    int fifoTail;
    int fifoSize;
    int *fifo;
    int *inqueue;
};

inline void dataPush(Data *data, int u) {
    data->fifo[data->fifoTail] = u;
    data->fifoTail = (data->fifoTail + 1) % data->V;
    data->fifoSize++;
    data->inqueue[u] = 1;
}

inline int dataPop(Data *data) {
    int retVal = -1;
    if (data->fifoSize > 0) {
        retVal = data->fifo[data->fifoHead];
        data->fifoHead = (data->fifoHead + 1) % data->V;
        data->fifoSize--;
        data->inqueue[retVal] = 0;
    }
    return retVal;
}

// applies if excess[u] > 0, residual[u * V + v] > 0, and height[u] = height[v] + 1
inline void push(Data *data, int u, int v) {
    int V = data->V;
    int delta = min(data->excess[u], data->residual[u * V + v]);
    if (delta) {
        data->residual[u * V + v] -= delta;
        data->residual[v * V + u] += delta;
        data->excess[u] -= delta;
        data->excess[v] += delta;
        if (!data->inqueue[v]) {
            dataPush(data, v);
        }
    }
}

// applies if excess[u] > 0 and if height[u] < height[v] for all (u,v) residual[u * V + v] > 0
inline void relabel(Data *data, int u) {
    int V = data->V;
    int minHeight = INT_MAX;
    for (int i = 0; i < data->nedge[u]; i++) {
        int v = data->edge[u * V + i];
        if (data->residual[u * V + v] > 0)
            minHeight = min(minHeight, data->height[v]);
    }
    data->height[u] = minHeight + 1;
}

inline void discharge(Data *data, int u) {
    int V = data->V;
    while (data->excess[u]) {
        relabel(data, u);
        for (int i = 0; i < data->nedge[u]; i++) {
            int v = data->edge[u * V + i];
            if (data->height[u] > data->height[v] && data->residual[u * V + v] > 0) {
                push(data, u, v);
                if (data->excess[u] == 0)
                    break;
            }
        }
    }
}
}  // namespace PR
using namespace PR;

void PushRelabel(Graph *graph, int *flow) {
    Data *data = (Data *)malloc(sizeof(Data));
    int V = data->V = graph->V;
    int S = data->S = graph->S;
    int T = data->T = graph->T;
    data->ncpus = graph->ncpus;
    data->capacity = (int *)malloc(sizeof(int) * V * V);
    data->edge = (int *)malloc(sizeof(int) * V * V);
    data->nedge = (int *)malloc(sizeof(int) * V);
    data->excess = (int *)malloc(sizeof(int) * V);
    data->residual = (int *)malloc(sizeof(int) * V * V);
    data->height = (int *)malloc(sizeof(int) * V);
    data->fifoHead = 0;
    data->fifoTail = 0;
    data->fifoSize = 0;
    data->fifo = (int *)malloc(sizeof(int) * V);
    data->inqueue = (int *)malloc(sizeof(int) * V);

    TIMING_START(_init);
    for (int u = 0; u < V; u++) {
        for (int v = 0; v < V; v++) {
            data->capacity[u * V + v] = 0;
            data->residual[u * V + v] = 0;
        }
    }
    for (int u = 0; u < V; u++) {
        data->nedge[u] = 0;
    }
    for (int u = 0; u < V; u++) {
        for (int i = 0; i < (int)graph->edge[u].size(); i++) {
            int v = graph->edge[u][i].first;
            data->edge[u * V + data->nedge[u]++] = v;
            data->edge[v * V + data->nedge[v]++] = u;
            data->capacity[u * V + v] = graph->edge[u][i].second;
            data->residual[u * V + v] = graph->edge[u][i].second;
        }
    }
    for (int u = 0; u < V; u++) {
        data->excess[u] = 0;
        data->height[u] = 0;
        data->inqueue[u] = 0;
    }
    TIMING_END(_init);

    TIMING_START(_shortest_path);
    for (int u = 0; u < V; u++) {
        data->height[u] = INT_MAX;
    }
    data->height[T] = 0;
    dataPush(data, T);
    for (int u; (u = dataPop(data)) != -1;) {
        for (int i = 0; i < data->nedge[u]; i++) {
            int v = data->edge[u * V + i];
            if (data->height[v] == INT_MAX && data->residual[u * V + v]) {
                data->height[v] = data->height[u] + 1;
                dataPush(data, v);
            }
        }
    }
    TIMING_END(_shortest_path);

    TIMING_START(_preflow);
    data->height[S] = V - 1;
    data->excess[S] = INT_MAX;
    for (int i = 0; i < data->nedge[S]; i++) {
        int v = data->edge[S * V + i];
        push(data, S, v);
    }
    TIMING_END(_preflow);

    TIMING_START(_innerPushRelabel);
    for (int u; (u = dataPop(data)) != -1;) {
        if (u != S && u != T)
            discharge(data, u);
    }
    TIMING_END(_innerPushRelabel);

    TIMING_START(_flow);
    for (int u = 0; u < V; u++) {
        for (int i = 0; i < (int)graph->edge[u].size(); i++) {
            int v = graph->edge[u][i].first;
            flow[u * V + v] = data->capacity[u * V + v] - data->residual[u * V + v];
        }
    }
    TIMING_END(_flow);

    free(data->capacity);
    free(data->edge);
    free(data->nedge);
    free(data->excess);
    free(data->residual);
    free(data->height);
    free(data->fifo);
    free(data->inqueue);
    free(data);
}