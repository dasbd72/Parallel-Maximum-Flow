#include "parallel-push-relabel.hh"

#include <omp.h>
#include <pthread.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <vector>

#include "graph.hh"
#include "utility.hh"

namespace PPR {
struct Data {
    int V;
    int S;
    int T;
    int ncpus;
    int *edge;
    int *nedge;
    int *excess;
    int *residual;
    int *height;
    int *queue;
    int queSize;
    int *inqueue;
    int *cnt;
#if QTYPE == 0
    int queFront;
    int queBack;
#elif QTYPE == 1
    int *label;
#elif QTYPE == 2
    int *label;
#elif QTYPE == 3
    int *label;
#elif QTYPE == 4
    int *label;
#endif
    pthread_mutex_t *vertexLock;
    pthread_mutex_t queLock;
};

inline int min(int x, int y) {
    if (x < y)
        return x;
    else
        return y;
}

inline void swap(int *x, int *y) {
    int tmp = *x;
    *x = *y;
    *y = tmp;
}

inline void quePush(Data *data, int u) {
    pthread_mutex_lock(&data->queLock);
#if QTYPE == 0
    data->queue[data->queBack] = u;
    data->queBack = (data->queBack + 1) % data->V;
    data->queSize++;
#elif QTYPE == 1 || QTYPE == 2 || QTYPE == 3
    data->queue[++data->queSize] = u;
    int idx = data->queSize;
    while (idx > 1 && data->label[data->queue[idx]] > data->label[data->queue[idx / 2]]) {
        swap(&data->queue[idx], &data->queue[idx / 2]);
        idx = idx / 2;
    }
#elif QTYPE == 4
    data->queue[++data->queSize] = u;
    int idx = data->queSize;
    while (idx > 1 && data->label[data->queue[idx]] < data->label[data->queue[idx / 2]]) {
        swap(&data->queue[idx], &data->queue[idx / 2]);
        idx = idx / 2;
    }
#endif
    pthread_mutex_unlock(&data->queLock);
}

inline int quePop(Data *data) {
    int retVal = -1;
    pthread_mutex_lock(&data->queLock);
#if QTYPE == 0
    if (data->queSize > 0) {
        retVal = data->queue[data->queFront];
        data->queFront = (data->queFront + 1) % data->V;
        data->queSize--;
    }
#elif QTYPE == 1 || QTYPE == 2 || QTYPE == 3
    if (data->queSize > 0) {
        retVal = data->queue[1];
        data->queue[1] = data->queue[data->queSize--];
        int idx = 1;
        while (idx * 2 + 1 <= data->queSize && (data->label[data->queue[idx]] < data->label[data->queue[idx * 2]] || data->label[data->queue[idx]] < data->label[data->queue[idx * 2 + 1]])) {
            if (data->label[data->queue[idx * 2]] > data->label[data->queue[idx * 2 + 1]]) {
                swap(&data->queue[idx], &data->queue[idx * 2]);
                idx = idx * 2;
            } else {
                swap(&data->queue[idx], &data->queue[idx * 2 + 1]);
                idx = idx * 2 + 1;
            }
        }
        if (idx * 2 <= data->queSize && data->label[data->queue[idx]] < data->label[data->queue[idx * 2]]) {
            swap(&data->queue[idx], &data->queue[idx * 2]);
        }
    }
#elif QTYPE == 4
    if (data->queSize > 0) {
        retVal = data->queue[1];
        data->queue[1] = data->queue[data->queSize--];
        int idx = 1;
        while (idx * 2 + 1 <= data->queSize && (data->label[data->queue[idx]] > data->label[data->queue[idx * 2]] || data->label[data->queue[idx]] > data->label[data->queue[idx * 2 + 1]])) {
            if (data->label[data->queue[idx * 2]] < data->label[data->queue[idx * 2 + 1]]) {
                swap(&data->queue[idx], &data->queue[idx * 2]);
                idx = idx * 2;
            } else {
                swap(&data->queue[idx], &data->queue[idx * 2 + 1]);
                idx = idx * 2 + 1;
            }
        }
        if (idx * 2 <= data->queSize && data->label[data->queue[idx]] > data->label[data->queue[idx * 2]]) {
            swap(&data->queue[idx], &data->queue[idx * 2]);
        }
    }
#endif
    pthread_mutex_unlock(&data->queLock);
    return retVal;
}

// applies if excess[u] > 0, residual[u * V + v] > 0, and height[u] = height[v] + 1
inline void push(Data *data, int u, int v) {
    int V = data->V;
    int delta = min(data->excess[u], data->residual[u * V + v]);
    data->residual[u * V + v] -= delta;
    data->residual[v * V + u] += delta;
    data->excess[u] -= delta;
    data->excess[v] += delta;
    if (!data->inqueue[v] && v != data->S && v != data->T) {
        data->inqueue[v] = 1;
        quePush(data, v);
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
    bool done = false;
    while (!done) {
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
                        data->inqueue[u] = 0;
                        done = true;
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
    for (int u; (u = quePop(data)) != -1;) {
        data->cnt[u]++;
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
    data->edge = (int *)malloc(sizeof(int) * V * V);
    data->nedge = (int *)malloc(sizeof(int) * V);
    data->excess = (int *)malloc(sizeof(int) * V);
    data->residual = (int *)malloc(sizeof(int) * V * V);
    data->height = (int *)malloc(sizeof(int) * V);
    data->vertexLock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * V);
    data->queue = (int *)malloc(sizeof(int) * (data->V + 1));
    data->queSize = 0;
    data->inqueue = (int *)malloc(sizeof(int) * data->V);
    data->cnt = (int *)malloc(sizeof(int) * data->V);
#if QTYPE == 0
    data->queFront = 0;
    data->queBack = 0;
#elif QTYPE == 1
    data->label = data->height;
#elif QTYPE == 2
    data->label = (int *)malloc(sizeof(int) * data->V);  // Distance
#elif QTYPE == 3
    data->label = (int *)malloc(sizeof(int) * data->V);  // Separates layer
#elif QTYPE == 4
    data->label = data->cnt;  // Appearance
#endif
    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * data->ncpus);
    for (int u = 0; u < V; u++) {
        data->vertexLock[u] = PTHREAD_MUTEX_INITIALIZER;
    }
    data->queLock = PTHREAD_MUTEX_INITIALIZER;

    TIMING_START(_init);
    {
        for (int u = 0; u < V; u++) {
            for (int v = 0; v < V; v++) {
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
                data->residual[u * V + v] = graph->edge[u][i].second;
            }
        }
        for (int u = 0; u < V; u++) {
            data->excess[u] = 0;
            data->height[u] = 0;
            data->inqueue[u] = 0;
            data->cnt[u] = 0;
        }
    }
    TIMING_END(_init);

    TIMING_START(_shortest_path);
    {
        for (int u = 0; u < V; u++) {
            data->height[u] = INT_MAX;
        }
        data->height[T] = 0;
        std::queue<int> que;
        que.push(T);
        while (que.size()) {
            int u = que.front();
            que.pop();
            for (int i = 0; i < data->nedge[u]; i++) {
                int v = data->edge[u * V + i];
                if (data->height[v] == INT_MAX && data->residual[u * V + v]) {
                    data->height[v] = data->height[u] + 1;
                    que.push(v);
                }
            }
        }
    }
    TIMING_END(_shortest_path);

#if QTYPE == 2
    {
        for (int u = 0; u < V; u++) {
            data->label[u] = data->height[u];
        }
    }
#elif QTYPE == 3
    {
        std::vector<int> num(V, 0);
        for (int u = 0; u < V; u++) {
            data->label[u] = num[data->height[u]]++;
        }
    }
#endif

    TIMING_START(_preflow);
    {
        data->height[S] = V - 1;
        data->excess[S] = INT_MAX;
        for (int i = 0; i < data->nedge[S]; i++) {
            int v = data->edge[S * V + i];
            if (data->residual[S * V + v] > 0) {
                push(data, S, v);
            }
        }
    }
    TIMING_END(_preflow);

    TIMING_START(_innerPushRelabel);
    {
        for (int tid = 0; tid < data->ncpus; tid++) {
            pthread_create(&threads[tid], 0, pushRelabelThread, data);
        }
        for (int tid = 0; tid < data->ncpus; tid++) {
            pthread_join(threads[tid], NULL);
        }
    }
    TIMING_END(_innerPushRelabel);

    {
        int sum = 0;
        for (int i = 0; i < V; i++) {
            sum += data->cnt[i];
        }
        printf("Ave: %d\n", sum / V);
    }

    TIMING_START(_flow);
    {
        for (int u = 0; u < V; u++) {
            for (int i = 0; i < (int)graph->edge[u].size(); i++) {
                int v = graph->edge[u][i].first;
                flow[u * V + v] = graph->edge[u][i].second - data->residual[u * V + v];
            }
        }
    }
    TIMING_END(_flow);

    for (int u = 0; u < V; u++) {
        pthread_mutex_destroy(&data->vertexLock[u]);
    }
    pthread_mutex_destroy(&data->queLock);
    free(data->edge);
    free(data->nedge);
    free(data->excess);
    free(data->residual);
    free(data->height);
    free(data->queue);
    free(data->inqueue);
    free(data->cnt);
#if QTYPE == 2
    free(data->label);
#elif QTYPE == 3
    free(data->label);
#endif
    free(data->vertexLock);
    free(data);
    free(threads);
}