#include "main_cornellbox.hpp"
#include "main_earth.hpp"
#include "main_final0.hpp"
#include "main_final1.hpp"
#include "main_light.hpp"
#include "main_mesh.hpp"
#include "main_perlin.hpp"
#include "main_shapes.hpp"

#include <cstdlib>

int main(int argc, char** argv) {
    if (argc < 2) return -1;
    int id = std::atoi(argv[1]);
    argc--, argv++;

    int (*entrance)(int, char**) = nullptr;
    switch (id) {
        case 0: entrance = demo::final0::main; break;
        case 1: entrance = demo::final1::main; break;
        case 2: entrance = demo::cornell::main; break;
        case 3: entrance = demo::earth::main; break;
        case 4: entrance = demo::perlin::main; break;
        case 5: entrance = demo::shapes::main; break;
        case 6: entrance = demo::mesh::main; break;
        case 7: entrance = demo::light::main; break;
        default: break;
    }
    if (!entrance) return -1;

    return entrance(argc, argv);
}
