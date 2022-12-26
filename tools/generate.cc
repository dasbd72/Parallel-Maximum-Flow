#include <cassert>
#include <iostream>
#include <random>
#include <thread>

int main(int argc, char** argv) {
    assert(argc == 3);

    int V = atoi(argv[1]);
    int E = 0;
    char* filename = argv[2];
    FILE* file;
    int* capacity = (int*)malloc(V * V * sizeof(int));
    int tmp[3];

    std::mt19937 generator, cap_generator;
    std::uniform_real_distribution<double> distribution(0, 1);
    std::uniform_int_distribution<int> cap_distribution(0, 10);

    for (int r = 0; r < V; r++) {
        for (int c = 0; c < V; c++) {
            if (r == c) {
                capacity[r * V + c] = 0;
            } else if (distribution(generator) < 0.75) {
                capacity[r * V + c] = cap_distribution(cap_generator);
            }
            if (capacity[r * V + c] != 0) {
                E++;
            }
        }
    }

    file = fopen(filename, "wb");
    fwrite(&V, sizeof(int), 1, file);
    fwrite(&E, sizeof(int), 1, file);
    for (int r = 0; r < V; r++) {
        for (int c = 0; c < V; c++) {
            if (capacity[r * V + c] != 0) {
                tmp[0] = r;
                tmp[1] = c;
                tmp[2] = capacity[r * V + c];
                fwrite(tmp, sizeof(int), 3, file);
            }
        }
    }
    fclose(file);

    free(capacity);
}