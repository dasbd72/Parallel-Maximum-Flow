#include "utility.hh"

#include <cstdio>

void print(int V, int *A) {
    for (int r = 0; r < V; r++) {
        for (int c = 0; c < V; c++) {
            printf("%d ", A[r * V + c]);
        }
        printf("\n");
    }
}

int min(int x, int y) {
    if (x < y)
        return x;
    else
        return y;
}
