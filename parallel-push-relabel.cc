#include "parallel-push-relabel.hh"

#include <omp.h>
#include <pthread.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "graph.hh"
#include "utility.hh"

namespace PPR {
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
    pthread_mutex_t *vertexLock;
    pthread_mutex_t fifoLock;
};

inline void dataPush(Data *data, int u) {
    pthread_mutex_lock(&data->fifoLock);
    data->fifo[data->fifoTail] = u;
    data->fifoTail = (data->fifoTail + 1) % data->V;
    data->fifoSize++;
    data->inqueue[u] = 1;
    pthread_mutex_unlock(&data->fifoLock);
}

inline int dataPop(Data *data) {
    int retVal = -1;
    pthread_mutex_lock(&data->fifoLock);
    if (data->fifoSize > 0) {
        retVal = data->fifo[data->fifoHead];
        data->fifoHead = (data->fifoHead + 1) % data->V;
        data->fifoSize--;
        data->inqueue[retVal] = 0;
    }
    pthread_mutex_unlock(&data->fifoLock);
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
        // Lock inside discharge to prevent holding
        pthread_mutex_lock(&data->vertexLock[u]);
        relabel(data, u);
        for (int i = 0; i < data->nedge[u]; i++) {
            int v = data->edge[u * V + i];
            if (data->height[u] > data->height[v] && data->residual[u * V + v] > 0) {
                // Use trylock to prevent deadlock
                if (pthread_mutex_trylock(&data->vertexLock[v]) == 0) {
                    push(data, u, v);
                    pthread_mutex_unlock(&data->vertexLock[v]);
                    if (data->excess[u] == 0) {
                        break;
                    }
                }
            }
        }
        pthread_mutex_unlock(&data->vertexLock[u]);
    }
}

void *pushRelabelThread(void *arg) {
    Data *data = (Data *)arg;
    int V = data->V;
    int S = data->S;
    int T = data->T;
    for (int u; (u = dataPop(data)) != -1;) {
        if (u != S && u != T)
            discharge(data, u);
    }
    return NULL;
}
}  // namespace PPR
using namespace PPR;

void ParallelPushRelabel(Graph *graph, int *flow) {
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
    data->vertexLock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * V);
    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * data->ncpus);

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

#pragma omp parallel for
    for (int u = 0; u < V; u++) {
        data->vertexLock[u] = PTHREAD_MUTEX_INITIALIZER;
    }
    data->fifoLock = PTHREAD_MUTEX_INITIALIZER;

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
    for (int tid = 0; tid < data->ncpus; tid++) {
        pthread_create(&threads[tid], 0, pushRelabelThread, data);
    }
    for (int tid = 0; tid < data->ncpus; tid++) {
        pthread_join(threads[tid], NULL);
    }
    TIMING_END(_innerPushRelabel);

#pragma omp parallel for
    for (int u = 0; u < V; u++) {
        pthread_mutex_destroy(&data->vertexLock[u]);
    }
    pthread_mutex_destroy(&data->fifoLock);

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
    free(data->vertexLock);
    free(data);
    free(threads);
}