#include "push-relabel.hh"

#include <omp.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>
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
    int *queue;
    int queSize;
    int *inqueue;
#if QTYPE == 0
    int queFront;
    int queBack;
#elif QTYPE == 1
    int *vertexPtr;
#elif QTYPE == 2
    int *distance;
#elif QTYPE == 3
    int *label;
#endif
};

inline void quePush(Data *data, int u) {
#if QTYPE == 0
    if (!data->inqueue[u]) {
        data->queue[data->queBack] = u;
        data->queBack = (data->queBack + 1) % data->V;
        data->queSize++;
        data->inqueue[u] = 1;
    }
#elif QTYPE == 1
    if (!data->inqueue[u]) {
        data->queue[++data->queSize] = u;
        data->vertexPtr[u] = data->queSize;
        int idx = data->queSize, tmp;
        while (idx > 1 && data->height[data->queue[idx]] > data->height[data->queue[idx / 2]]) {
            data->vertexPtr[data->queue[idx]] = idx / 2;
            data->vertexPtr[data->queue[idx / 2]] = idx;
            tmp = data->queue[idx];
            data->queue[idx] = data->queue[idx / 2];
            data->queue[idx / 2] = tmp;
            idx = idx / 2;
        }
        data->inqueue[u] = 1;
    }
#elif QTYPE == 2
    if (!data->inqueue[u]) {
        data->queue[++data->queSize] = u;
        int idx = data->queSize, tmp;
        while (idx > 1 && data->distance[data->queue[idx]] > data->distance[data->queue[idx / 2]]) {
            tmp = data->queue[idx];
            data->queue[idx] = data->queue[idx / 2];
            data->queue[idx / 2] = tmp;
            idx = idx / 2;
        }
        data->inqueue[u] = 1;
    }
#elif QTYPE == 3
    if (!data->inqueue[u]) {
        data->queue[++data->queSize] = u;
        int idx = data->queSize, tmp;
        while (idx > 1 && data->label[data->queue[idx]] > data->label[data->queue[idx / 2]]) {
            tmp = data->queue[idx];
            data->queue[idx] = data->queue[idx / 2];
            data->queue[idx / 2] = tmp;
            idx = idx / 2;
        }
        data->inqueue[u] = 1;
    }
#endif
}

inline int quePop(Data *data) {
    int retVal = -1;
#if QTYPE == 0
    if (data->queSize > 0) {
        retVal = data->queue[data->queFront];
        data->queFront = (data->queFront + 1) % data->V;
        data->queSize--;
        data->inqueue[retVal] = 0;
    }
#elif QTYPE == 1
    if (data->queSize > 0) {
        retVal = data->queue[1];
        data->queue[1] = data->queue[data->queSize--];
        int idx = 1, tmp;
        while (idx * 2 + 1 <= data->queSize && (data->height[data->queue[idx]] < data->height[data->queue[idx * 2]] || data->height[data->queue[idx]] < data->height[data->queue[idx * 2 + 1]])) {
            if (data->height[data->queue[idx * 2]] > data->height[data->queue[idx * 2 + 1]]) {
                data->vertexPtr[data->queue[idx]] = idx * 2;
                data->vertexPtr[data->queue[idx * 2]] = idx;
                tmp = data->queue[idx];
                data->queue[idx] = data->queue[idx * 2];
                data->queue[idx * 2] = tmp;
                idx = idx * 2;
            } else {
                data->vertexPtr[data->queue[idx]] = idx * 2 + 1;
                data->vertexPtr[data->queue[idx * 2 + 1]] = idx;
                tmp = data->queue[idx];
                data->queue[idx] = data->queue[idx * 2 + 1];
                data->queue[idx * 2 + 1] = tmp;
                idx = idx * 2 + 1;
            }
        }
        if (idx * 2 <= data->queSize && data->height[data->queue[idx]] < data->height[data->queue[idx * 2]]) {
            data->vertexPtr[data->queue[idx]] = idx * 2;
            data->vertexPtr[data->queue[idx * 2]] = idx;
            tmp = data->queue[idx];
            data->queue[idx] = data->queue[idx * 2];
            data->queue[idx * 2] = tmp;
        }
        data->inqueue[retVal] = 0;
    }
#elif QTYPE == 2
    if (data->queSize > 0) {
        retVal = data->queue[1];
        data->queue[1] = data->queue[data->queSize--];
        int idx = 1, tmp;
        while (idx * 2 + 1 <= data->queSize && (data->distance[data->queue[idx]] < data->distance[data->queue[idx * 2]] || data->distance[data->queue[idx]] < data->distance[data->queue[idx * 2 + 1]])) {
            if (data->distance[data->queue[idx * 2]] > data->distance[data->queue[idx * 2 + 1]]) {
                tmp = data->queue[idx];
                data->queue[idx] = data->queue[idx * 2];
                data->queue[idx * 2] = tmp;
                idx = idx * 2;
            } else {
                tmp = data->queue[idx];
                data->queue[idx] = data->queue[idx * 2 + 1];
                data->queue[idx * 2 + 1] = tmp;
                idx = idx * 2 + 1;
            }
        }
        if (idx * 2 <= data->queSize && data->distance[data->queue[idx]] < data->distance[data->queue[idx * 2]]) {
            tmp = data->queue[idx];
            data->queue[idx] = data->queue[idx * 2];
            data->queue[idx * 2] = tmp;
        }
        data->inqueue[retVal] = 0;
    }
#elif QTYPE == 3
    if (data->queSize > 0) {
        retVal = data->queue[1];
        data->queue[1] = data->queue[data->queSize--];
        int idx = 1, tmp;
        while (idx * 2 + 1 <= data->queSize && (data->label[data->queue[idx]] < data->label[data->queue[idx * 2]] || data->label[data->queue[idx]] < data->label[data->queue[idx * 2 + 1]])) {
            if (data->label[data->queue[idx * 2]] > data->label[data->queue[idx * 2 + 1]]) {
                tmp = data->queue[idx];
                data->queue[idx] = data->queue[idx * 2];
                data->queue[idx * 2] = tmp;
                idx = idx * 2;
            } else {
                tmp = data->queue[idx];
                data->queue[idx] = data->queue[idx * 2 + 1];
                data->queue[idx * 2 + 1] = tmp;
                idx = idx * 2 + 1;
            }
        }
        if (idx * 2 <= data->queSize && data->label[data->queue[idx]] < data->label[data->queue[idx * 2]]) {
            tmp = data->queue[idx];
            data->queue[idx] = data->queue[idx * 2];
            data->queue[idx * 2] = tmp;
        }
        data->inqueue[retVal] = 0;
    }
#endif
    return retVal;
}

inline void queIncreaseKey(Data *data, int u) {
#if QTYPE == 1
    if (data->inqueue[u]) {
        int idx = data->vertexPtr[u], tmp;
        while (idx > 1 && data->height[data->queue[idx]] > data->height[data->queue[idx / 2]]) {
            tmp = data->queue[idx];
            data->queue[idx] = data->queue[idx / 2];
            data->queue[idx / 2] = tmp;
            idx = idx / 2;
        }
    }
#endif
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
#if QTYPE == 1
    queIncreaseKey(data, u);
#endif
}

inline void discharge(Data *data, int u) {
    int V = data->V;
    while (data->excess[u]) {
        // Lock inside discharge to prevent holding
        relabel(data, u);
        for (int i = 0; i < data->nedge[u]; i++) {
            int v = data->edge[u * V + i];
            if (data->height[u] > data->height[v] && data->residual[u * V + v] > 0) {
                // Use trylock to prevent deadlock
                push(data, u, v);
                if (data->excess[u] == 0) {
                    break;
                }
            }
        }
    }
}

void *pushRelabelThread(void *arg) {
    Data *data = (Data *)arg;
    int V = data->V;
    int S = data->S;
    int T = data->T;
    for (int u; (u = quePop(data)) != -1;) {
        if (u != S && u != T)
            discharge(data, u);
    }
    return NULL;
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
    data->queue = (int *)malloc(sizeof(int) * (data->V + 1));
    data->queSize = 0;
    data->inqueue = (int *)malloc(sizeof(int) * data->V);
#if QTYPE == 0
    data->queFront = 0;
    data->queBack = 0;
#elif QTYPE == 1
    data->vertexPtr = (int *)malloc(sizeof(int) * data->V);
#elif QTYPE == 2
    data->distance = (int *)malloc(sizeof(int) * data->V);
#elif QTYPE == 3
    data->label = (int *)malloc(sizeof(int) * data->V);
#endif

    TIMING_START(_init);
    {
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
            data->distance[u] = data->height[u];
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
            push(data, S, v);
        }
    }
    TIMING_END(_preflow);

    TIMING_START(_innerPushRelabel);
    pushRelabelThread(data);
    TIMING_END(_innerPushRelabel);

    TIMING_START(_flow);
    {
        for (int u = 0; u < V; u++) {
            for (int i = 0; i < (int)graph->edge[u].size(); i++) {
                int v = graph->edge[u][i].first;
                flow[u * V + v] = data->capacity[u * V + v] - data->residual[u * V + v];
            }
        }
    }
    TIMING_END(_flow);

    free(data->capacity);
    free(data->edge);
    free(data->nedge);
    free(data->excess);
    free(data->residual);
    free(data->height);
    free(data->queue);
    free(data->inqueue);
#if QTYPE == 0
#elif QTYPE == 1
    free(data->vertexPtr);
#elif QTYPE == 2
    free(data->distance);
#elif QTYPE == 3
    free(data->label);
#endif
    free(data);
}