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
};

inline void fifoPush(Data *data, int u) {
    data->fifo[data->fifoTail] = u;
    data->fifoTail = (data->fifoTail + 1) % data->V;
    data->fifoSize++;
}

inline int fifoPop(Data *data) {
    int retVal = data->fifo[data->fifoHead];
    data->fifoHead = (data->fifoHead + 1) % data->V;
    data->fifoSize--;
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
        if (data->excess[v] == 0 && v != data->S && v != data->T) {
            fifoPush(data, v);
        }
        data->excess[v] += delta;
    }
}

// applies if excess[u] > 0 and if height[u] < height[v] for all (u,v) residual[u * V + v] > 0
inline void relabel(Data *data, int u) {
    int V = data->V;
    int minHeight = INT_MAX;
    for (int v = 0; v < V; v++) {
        if (data->residual[u * V + v] > 0)
            minHeight = min(minHeight, data->height[v]);
    }
    data->height[u] = minHeight + 1;
}

inline int getExcess(Data *data) {
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

    TIMING_START(_init);
    for (int u = 0; u < V; u++) {
        for (int v = 0; v < V; v++) {
            data->capacity[u * V + v] = 0;
            data->residual[u * V + v] = 0;
        }
    }
    for (int u = 0; u < V; u++) {
        data->nedge[u] = graph->edge[u].size();
        for (int i = 0; i < data->nedge[u]; i++) {
            int v = graph->edge[u][i].first;
            data->edge[u * V + i] = v;
            data->capacity[u * V + v] = graph->edge[u][i].second;
            data->residual[u * V + v] = graph->edge[u][i].second;
        }
    }
    for (int u = 0; u < V; u++) {
        data->excess[u] = 0;
        data->height[u] = 0;
    }
    TIMING_END(_init);

    TIMING_START(_preflow);
    data->height[S] = V;
    data->excess[S] = INT_MAX;
    for (int i = 0; i < data->nedge[S]; i++) {
        int v = data->edge[S * V + i];
        int delta = data->residual[S * V + v];
        if (delta) {
            data->residual[S * V + v] = 0;
            data->residual[v * V + S] += delta;
            data->excess[S] -= delta;
            if (v != T) {
                fifoPush(data, v);
            }
            data->excess[v] += delta;
        }
    }
    TIMING_END(_preflow);

    TIMING_START(_innerPushRelabel);
    while (data->fifoSize) {
        int u = fifoPop(data);
        bool isMin = true;
        for (int v = 0; v < V; v++) {
            if (data->residual[u * V + v] > 0) {
                if (data->height[u] > data->height[v]) {
                    push(data, u, v);
                    isMin = false;
                    if (data->excess[u] == 0) {
                        break;
                    }
                }
            }
        }
        if (isMin) {
            relabel(data, u);
        }
        if (data->excess[u] > 0) {
            fifoPush(data, u);
        }
    }
    TIMING_END(_innerPushRelabel);

    TIMING_START(_flow);
    for (int u = 0; u < V; u++) {
        for (int i = 0; i < data->nedge[u]; i++) {
            int v = data->edge[u * V + i];
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
    free(data);
}