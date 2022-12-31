#include <cassert>
#include <cstring>
#include <iostream>
#include <queue>

enum {
    SUCCESS,
    SELF_CYCLE,
    NEGATIVE_FLOW,
    CAPACITY_EXCEED,
    NETFLOW_NONZERO,
    REACHED_TARGET
};

int verify(int V, int S, int T, int *capacity, int *flow, bool *visit, int *sum) {
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

int main(int argc, char **argv) {
    assert(argc == 3);

    char *input_filename = argv[1];
    char *output_filename = argv[2];
    FILE *input_file;
    FILE *output_file;
    int V, E;
    int S, T;
    int *capacity;  // Stores input
    int *flow;      // Stores output
    bool *visit;
    int *sum;
    int tmp[3];

    // Input file
    input_file = fopen(input_filename, "rb");
    fread(&V, sizeof(int), 1, input_file);
    fread(&E, sizeof(int), 1, input_file);
    fread(&S, sizeof(int), 1, input_file);
    fread(&T, sizeof(int), 1, input_file);
    capacity = (int *)malloc(V * V * sizeof(int));
    for (int i = 0; i < V * V; i++) {
        capacity[i] = 0;
    }
    for (int e = 0; e < E; e++) {
        fread(tmp, sizeof(int), 3, input_file);
        capacity[tmp[0] * V + tmp[1]] = tmp[2];
    }
    fclose(input_file);

    // Output file
    flow = (int *)malloc(V * V * sizeof(int));
    output_file = fopen(output_filename, "rb");
    fread(flow, sizeof(int), V * V, output_file);
    fclose(output_file);

    // Verify
    visit = (bool *)malloc(sizeof(bool) * V);
    sum = (int *)malloc(sizeof(int) * V);
    int err = verify(V, S, T, capacity, flow, visit, sum);
    if (err == SUCCESS) {
        printf("passed\n");
    } else {
        printf("failed ouo\n");
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
    }

    // Finalize
    free(capacity);
    free(flow);
    free(sum);
    return 0;
}