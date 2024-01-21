#pragma once

#include <vector>
#include <span>
#include <glm/vec3.hpp>
#include "vk_types.hpp"
#include "vk_mesh.hpp"


namespace MeshLoader {
    bool LoadObj(const char* path, std::vector<Vertex> *vertices, std::vector<uint32_t> *indices);
    bool LoadGltf(const char* path, std::span<Vertex> *vertices, std::span<uint32_t> *indices);
}
