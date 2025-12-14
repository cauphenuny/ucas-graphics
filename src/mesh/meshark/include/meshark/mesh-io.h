//
// Created by creeper on 7/21/24.
//

#ifndef MESHSIMPLIFICATION_MESHARK_INCLUDE_MESHARK_MESH_IO_H_
#define MESHSIMPLIFICATION_MESHARK_INCLUDE_MESHARK_MESH_IO_H_

#include <filesystem>
#include <meshark/geometry-mesh.h>

namespace meshark {
struct WavefrontObj {
    struct FaceVertex {
        int v{-1}; // vertex index
        std::optional<int> vt; // texture index
        std::optional<int> vn; // normal index
    };
    std::vector<int> face_splits; // indicates the start index of each face in face_vertices
    std::vector<FaceVertex> face_vertices; // array of all face vertices
    std::vector<glm::vec3> positions; // vertex positions
    std::vector<glm::vec2> uvs; // texture coordinates
    std::vector<glm::vec3> normals; // vertex normals
};

std::unique_ptr<WavefrontObj> readWavefrontObj(const std::filesystem::path& path);
std::unique_ptr<GeometryMesh> readGeometryMeshFromWavefrontObj(const std::filesystem::path& path);
}  // namespace meshark
#endif  // MESHSIMPLIFICATION_MESHARK_INCLUDE_MESHARK_MESH_IO_H_
