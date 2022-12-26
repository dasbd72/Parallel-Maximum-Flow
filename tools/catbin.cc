#include <cassert>
#include <cstdlib>
#include <iostream>
#include <random>
#include <thread>

int main(int argc, char** argv) {
    assert(argc == 2);

    char* filename = argv[1];
    FILE* file;
    int tmp;

    file = fopen(filename, "rb");
    for (int i = 0; fread(&tmp, sizeof(int), 1, file) > 0; i++) {
        std::cout << i << ": " << tmp << "\n";
    }
    fclose(file);
}