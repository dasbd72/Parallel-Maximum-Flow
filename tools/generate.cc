#include <cassert>
#include <cstring>
#include <iostream>
#include <random>
#include <thread>

int main(int argc, char** argv) {
    assert(argc == 4);

    int V = atoi(argv[1]);
    double D = (double)atoi(argv[2]) / 100.0;
    int E = 0;
    int S;
    int T;
    char* filename = argv[3];
    FILE* file;
    int* capacity = (int*)malloc(V * V * sizeof(int));
    int tmp[3];

    std::mt19937 generator, cap_generator, st_generator;
    std::uniform_real_distribution<double> distribution(0, 1);
    std::uniform_int_distribution<int> cap_distribution(0, 1000);
    std::uniform_int_distribution<int> st_distribution(0, V - 1);

    S = st_distribution(st_generator);
    do {
        T = st_distribution(st_generator);
    } while (T == S);

    memset(capacity, 0, sizeof(int) * V * V);
    // one-way edge
    for (int r = 0; r < V; r++) {
        for (int c = 0; c < r; c++) {
            double prob = distribution(generator);
            int cap = cap_distribution(cap_generator);
            if (cap > 0) {
                if (prob < D) {
                    E++;
                    if (prob < D / 2) {
                        capacity[r * V + c] = cap;
                    } else {
                        capacity[c * V + r] = cap;
                    }
                }
            }
        }
    }

    file = fopen(filename, "wb");
    fwrite(&V, sizeof(int), 1, file);
    fwrite(&E, sizeof(int), 1, file);
    fwrite(&S, sizeof(int), 1, file);
    fwrite(&T, sizeof(int), 1, file);
    for (int r = 0; r < V; r++) {
        for (int c = 0; c < V; c++) {
            if (capacity[r * V + c] != 0) {
                tmp[0] = r;
                tmp[1] = c;
                tmp[2] = capacity[r * V + c];
                fwrite(tmp, sizeof(int), 3, file);
                E--;
            }
        }
    }
    fclose(file);
    assert(E == 0);

    free(capacity);
}