#include "graph.hh"

#include <omp.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <queue>
#include <vector>

Graph::Graph(int argc, char **argv) {
    assert(argc == 3);
    V = atoi(argv[1]);
    D = (double)atoi(argv[2]) / 100.0;
    ncpus = omp_get_max_threads();
}

inline int randInt(int min, int max) {
    return rand() % (max - min + 1) + min;
}

inline double randDouble(double min, double max) {
    return rand() * (max - min) / (RAND_MAX + 1.0) + min;
}

void dfs(int u, int &time, std::vector<int> &d, std::vector<int> &f, std::vector<int> &c, std::vector<std::vector<std::pair<int, int>>> &raw, std::vector<std::vector<std::pair<int, int>>> &edge) {
    c[u] = 1;
    time = time + 1;
    d[u] = time;
    for (auto e : raw[u]) {
        if (c[e.first] != 2) {
            edge[u].emplace_back(e);
        }
        if (c[e.first] == 0) {
            dfs(e.first, time, d, f, c, raw, edge);
        }
    }
    c[u] = 2;
    time = time + 1;
    f[u] = time;
}

void Graph::generate() {
    srand(17 ^ V);

    S = randInt(0, V - 1);
    do {
        T = randInt(0, V - 1);
    } while (T == S);

    std::vector<std::vector<std::pair<int, int>>> raw(V);
    // one-way edge
#ifdef GRAPH_ONE_WAY
    for (int r = 0; r < V; r++) {
        for (int c = 0; c < r; c++) {
            double prob = randDouble(0, 1);
            if (prob < D) {
                int cap = randInt(0, 10000);
                if (cap > 0) {
                    if (prob < D / 2) {
                        raw[r].emplace_back(c, cap);
                    } else {
                        raw[c].emplace_back(r, cap);
                    }
                }
            }
        }
    }
#else
    for (int r = 0; r < V; r++) {
        for (int c = 0; c < V; c++) {
            double prob = randDouble(0, 1);
            if (prob < D) {
                int cap = randInt(0, 10000);
                if (cap > 0) {
                    if (prob < D / 2) {
                        raw[r].emplace_back(c, cap);
                    } else {
                        raw[c].emplace_back(r, cap);
                    }
                }
            }
        }
    }
#endif

#ifdef GRAPH_ACYCLIC
    edge.resize(V);
    // Acyclic edge
    int time = 0;
    std::vector<int> d(V, 0);
    std::vector<int> f(V, 0);
    std::vector<int> c(V, 0);
    dfs(S, time, d, f, c, raw, edge);
#else
    edge = raw;
#endif

    // count & capacity
    E = 0;
    for (int u = 0; u < V; u++) {
        E += edge[u].size();
    }
}

enum {
    SUCCESS,
    SELF_CYCLE,
    NEGATIVE_FLOW,
    CAPACITY_EXCEED,
    NETFLOW_NONZERO,
    REACHED_TARGET
};

inline int check(int V, int S, int T, int *capacity, bool *visit, int *sum, int *flow) {
    memset(sum, 0, sizeof(int) * V);
    for (int r = 0; r < V; r++) {
        for (int c = 0; c < V; c++) {
            if (r == c) {
                if (!(flow[r * V + c] == 0))
                    return SELF_CYCLE;
            } else {
                if (!(flow[r * V + c] >= 0))
                    return NEGATIVE_FLOW;
                if (!(flow[r * V + c] <= capacity[r * V + c]))
                    return CAPACITY_EXCEED;
                capacity[r * V + c] -= flow[r * V + c];
                capacity[c * V + r] += flow[r * V + c];
            }
            sum[r] -= flow[r * V + c];
            sum[c] += flow[r * V + c];
        }
    }

    for (int i = 0; i < V; i++) {
        if (i != S && i != T && sum[i] != 0) {
            return NETFLOW_NONZERO;
        }
    }

    memset(visit, 0, sizeof(bool) * V);
    std::queue<int> q;
    q.emplace(S);
    visit[S] = true;

    while (!q.empty()) {
        int u = q.front();
        q.pop();
        for (int v = 0; v < V; v++) {
            if (!visit[v] && capacity[u * V + v] > 0) {
                visit[v] = true;
                q.emplace(v);
                if (v == T)
                    return REACHED_TARGET;
            }
        }
    }

    return SUCCESS;
}

void Graph::verify(int *flow) {
    bool *visit = (bool *)malloc(sizeof(bool) * V);
    int *sum = (int *)malloc(sizeof(int) * V);
    int *capacity = (int *)malloc(V * V * sizeof(int));
    int err;

    memset(capacity, 0, sizeof(int) * V * V);
    for (int u = 0; u < V; u++) {
        for (auto e : edge[u]) {
            capacity[u * V + e.first] = e.second;
        }
    }
    err = check(V, S, T, capacity, visit, sum, flow);
    if (err == SUCCESS) {
        printf("\033[1;32m");
        printf("Passed.\n");
        printf("\033[0m");
    } else {
        printf("\033[1;31m");
        printf("Failed.\n");
        switch (err) {
            case SELF_CYCLE:
                printf("SELF_CYCLE\n");
                break;
            case NEGATIVE_FLOW:
                printf("NEGATIVE_FLOW\n");
                break;
            case CAPACITY_EXCEED:
                printf("CAPACITY_EXCEED\n");
                break;
            case NETFLOW_NONZERO:
                printf("NETFLOW_NONZERO\n");
                break;
            case REACHED_TARGET:
                printf("REACHED_TARGET\n");
                break;
        }
        printf("\033[0m");
    }

    // Finalize
    free(capacity);
    free(visit);
    free(sum);
}
