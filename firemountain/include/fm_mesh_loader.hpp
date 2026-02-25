#pragma once

#include <optional>
#include <EASTL/vector.h>
#include <glm/vec3.hpp>
#include "vk_buffer.hpp"
#include "vk_image.hpp"
#include "vk_mesh.hpp"
#include "vk_renderer.hpp"


struct LoadedGLTF : public IRenderable {
    ankerl::unordered_dense::map<eastl::string, std::shared_ptr<MeshAsset>> meshes;
    ankerl::unordered_dense::map<eastl::string, std::shared_ptr<Node>> nodes;
    ankerl::unordered_dense::map<eastl::string, fmvk::Image::AllocatedImage> images;
    ankerl::unordered_dense::map<eastl::string, std::shared_ptr<GLTFMaterial>> materials;

    eastl::vector<std::shared_ptr<Node>> top_nodes;

    eastl::vector<VkSampler> samplers;

    DescriptorAllocatorGrowable descriptor_pool;

    fmvk::Buffer::AllocatedBuffer material_data_buffer;

    fmvk::Vulkan* creator;

    ~LoadedGLTF() { clear_all(); };
    virtual void Draw(const glm::mat4& top_matrix, DrawContext& ctx);

private:
    void clear_all();
};


namespace MeshLoader {
    bool LoadObj(const char* path, eastl::vector<Vertex> *vertices, eastl::vector<uint32_t> *indices);
    std::optional<std::shared_ptr<LoadedGLTF>> load_GLTF(fmvk::Vulkan* engine, eastl::string_view file_path);

    // eastl::vector<std::shared_ptr<MeshAsset>> LoadGltf(std::filesystem::path file_path, fmvk::Vulkan *vk_engine);

}
