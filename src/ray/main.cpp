#include "main_earth.hpp"
#include "main_spheres.hpp"

#include <cstdlib>

int main(int argc, char** argv) {
    if (argc < 2) return -1;
    int id = std::atoi(argv[1]);
    argc--, argv++;

    int (*handler)(int, char**) = nullptr;
    switch (id) {
        case 1: handler = demo::spheres::main; break;
        case 2: handler = demo::earth::main; break;
        default: break;
    }
    if (!handler) return -1;

    return handler(argc, argv);
}
