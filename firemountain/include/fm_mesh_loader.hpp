#pragma once

#include <filesystem>
#include <vector>
#include <span>
#include <glm/vec3.hpp>
#include "vk_types.hpp"
#include "vk_mesh.hpp"
#include "vk_renderer.hpp"


namespace MeshLoader {
    bool LoadObj(const char* path, std::vector<Vertex> *vertices, std::vector<uint32_t> *indices);
    std::vector<std::shared_ptr<MeshAsset>> LoadGltf(std::filesystem::path file_path, fmVK::Vulkan *vk_engine);
}
