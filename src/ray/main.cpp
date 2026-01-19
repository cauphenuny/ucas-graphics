#include "main_cornellbox.hpp"
#include "main_earth.hpp"
#include "main_light.hpp"
#include "main_mesh.hpp"
#include "main_perlin.hpp"
#include "main_shapes.hpp"
#include "main_spheres.hpp"

#include <cstdlib>

int main(int argc, char** argv) {
    if (argc < 2) return -1;
    int id = std::atoi(argv[1]);
    argc--, argv++;

    int (*entrance)(int, char**) = nullptr;
    switch (id) {
        case 1: entrance = demo::spheres::main; break;
        case 2: entrance = demo::earth::main; break;
        case 3: entrance = demo::perlin::main; break;
        case 4: entrance = demo::shapes::main; break;
        case 5: entrance = demo::mesh::main; break;
        case 6: entrance = demo::light::main; break;
        case 7: entrance = demo::cornell::main; break;
        default: break;
    }
    if (!entrance) return -1;

    return entrance(argc, argv);
}
