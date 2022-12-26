#include <cassert>
#include <cstring>
#include <iostream>
#include <queue>

int verify(int V, int S, int T, int *capacity, int *flow, bool *visit) {
    for (int r = 0; r < V; r++) {
        for (int c = 0; c < V; c++) {
            if (r == c) {
                if (!(flow[r * V + c] == 0))
                    return 1;
            } else {
                if (!(flow[r * V + c] <= capacity[r * V + c]))
                    return 2;
                capacity[r * V + c] -= flow[r * V + c];
            }
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
                    return 3;
            }
        }
    }

    return 0;
}

int main(int argc, char **argv) {
    assert(argc == 5);

    char *input_filename = argv[1];
    char *output_filename = argv[2];
    FILE *input_file;
    FILE *output_file;
    int V, E;
    int S = atoi(argv[3]);
    int T = atoi(argv[4]);
    int *capacity;  // Stores input
    int *flow;      // Stores output
    bool *visit;
    int tmp[3];

    // Input file
    input_file = fopen(input_filename, "rb");
    fread(&V, sizeof(int), 1, input_file);
    fread(&E, sizeof(int), 1, input_file);
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
    int err = verify(V, S, T, capacity, flow, visit);
    if (err == 0) {
        printf("passed\n");
    } else {
        printf("failed ouo\n");
        printf("error: %d\n", err);
    }

    // Finalize
    free(capacity);
    free(flow);
    return 0;
}