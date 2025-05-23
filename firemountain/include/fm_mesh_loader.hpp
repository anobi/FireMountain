#pragma once

#include <optional>
#include <filesystem>
#include <vector>
#include <span>
#include <glm/vec3.hpp>
#include "vk_types.hpp"
#include "vk_buffer.hpp"
#include "vk_image.hpp"
#include "vk_mesh.hpp"
#include "vk_renderer.hpp"


struct LoadedGLTF : public IRenderable {
    std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::unordered_map<std::string, fmvk::Image::AllocatedImage> images;
    std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

    std::vector<std::shared_ptr<Node>> top_nodes;

    std::vector<VkSampler> samplers;

    DescriptorAllocatorGrowable descriptor_pool;

    fmvk::Buffer::AllocatedBuffer material_data_buffer;

    fmvk::Vulkan* creator;

    ~LoadedGLTF() { clear_all(); };
    virtual void Draw(const glm::mat4& top_matrix, DrawContext& ctx);

private:
    void clear_all();
};


namespace MeshLoader {
    bool LoadObj(const char* path, std::vector<Vertex> *vertices, std::vector<uint32_t> *indices);
    std::optional<std::shared_ptr<LoadedGLTF>> load_GLTF(fmvk::Vulkan* engine, std::string_view file_path);

    // std::vector<std::shared_ptr<MeshAsset>> LoadGltf(std::filesystem::path file_path, fmvk::Vulkan *vk_engine);

}