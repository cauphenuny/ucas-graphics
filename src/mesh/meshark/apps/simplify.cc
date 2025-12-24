#include <iostream>
#include <meshark/mesh-io.h>
#include <meshark/mesh-simplifier.h>
#include <spdlog/spdlog.h>
using namespace meshark;

int main(int argc, char** argv) {
    if (argc != 4 && argc != 5) {
        std::cerr << "Usage: " << argv[0]
                  << " <input obj path> <output obj path> <ratio|num_edges> [-v|-vv]" << std::endl;
        return 1;
    }
    if (argc == 5) {
        if (std::string(argv[4]) == "-v") spdlog::set_level(spdlog::level::debug);
        if (std::string(argv[4]) == "-vv") spdlog::set_level(spdlog::level::trace);
    } else {
        spdlog::set_level(spdlog::level::info);
    }
    auto mesh = readGeometryMeshFromWavefrontObj(argv[1]);
    std::unique_ptr<MeshSimplifier> simplifier = std::make_unique<MeshSimplifier>(*mesh);
    double ratio = std::stod(argv[3]);
    std::variant<int, Real> target;
    if (ratio < 1) {
        target = ratio;
    } else {
        target = static_cast<int>(ratio);
    }
    simplifier->runSimplify(target);
    mesh->writeWavefrontObj(argv[2]);
}
