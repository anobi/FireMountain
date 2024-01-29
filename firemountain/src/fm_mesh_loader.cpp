#include <tiny_obj_loader.h>
#include <fmt/core.h>

#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>

#include "fm_mesh_loader.hpp"


const int FACE_VERTICES = 3;
constexpr bool OVERRIDE_COLORS = true;


bool MeshLoader::LoadObj(const char *path, std::vector<Vertex> *vertices, std::vector<uint32_t> *indices)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warnings;
    std::string errors;
    tinyobj::LoadObj(&attrib, &shapes, &materials, &warnings, &errors, path, nullptr);
    if(!warnings.empty()) {
        fmt::println("WARNING: {}", warnings);
    }
    if(!errors.empty()) {
        fmt::println("{}", errors);
        return false;
    }

    for (size_t s = 0; s < shapes.size(); s++) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            for (size_t v = 0; v < FACE_VERTICES; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                indices->push_back(idx.vertex_index);

                tinyobj::real_t vertex_x = attrib.vertices[3 * idx.vertex_index + 0];
                tinyobj::real_t vertex_y = attrib.vertices[3 * idx.vertex_index + 1];
                tinyobj::real_t vertex_z = attrib.vertices[3 * idx.vertex_index + 2];

                tinyobj::real_t normal_x = attrib.normals[3 * idx.normal_index + 0];
                tinyobj::real_t normal_y = attrib.normals[3 * idx.normal_index + 1];
                tinyobj::real_t normal_z = attrib.normals[3 * idx.normal_index + 2];

                Vertex vertex = {
                    .position = {vertex_x, vertex_y, vertex_z},
                    .normal = {normal_x, normal_y, normal_z},
                    .color = {normal_x, normal_y, normal_z, 1.0f}
                };

                vertices->push_back(vertex);
            }
            index_offset += FACE_VERTICES;
        }
    }
    return true;
}

std::vector<std::shared_ptr<MeshAsset>> MeshLoader::LoadGltf(std::filesystem::path file_path, fmVK::Vulkan *vk_engine)
{
    fmt::println("Loading GLTF: {}", file_path.string());

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(file_path);

    constexpr auto gltf_options = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

    fastgltf::Asset gltf;
    fastgltf::Parser parser {};

    auto load = parser.loadGltfBinary(&data, file_path.parent_path(), gltf_options);
    if (load) {
        gltf = std::move(load.get());
    } else {
        fmt::println("!! Failed to load glTF: {}", fastgltf::to_underlying(load.error()));
        return {};
    }

    std::vector<std::shared_ptr<MeshAsset>> meshes;
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    for (fastgltf::Mesh& mesh : gltf.meshes) {
        MeshAsset mesh_asset;
        mesh_asset.name = mesh.name;
        indices.clear();
        vertices.clear();

        for (auto&& p : mesh.primitives) {
            GeoSurface surface;
            surface.start_index = (uint32_t) indices.size();
            surface.count = (uint32_t) gltf.accessors[p.indicesAccessor.value()].count;
            size_t initial_vertex = vertices.size();

            // Load indices
            {
                fastgltf::Accessor& index_accessor = gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + index_accessor.count);
                fastgltf::iterateAccessor<uint32_t>(gltf, index_accessor,
                    [&](uint32_t idx) {
                        indices.push_back(idx + initial_vertex);
                });
            }

            // Load vertex positions
            {
                fastgltf::Accessor& position_accessor = gltf.accessors[p.findAttribute("POSITION")->second];
                vertices.resize(vertices.size() + position_accessor.count);
                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    gltf,
                    position_accessor,
                    [&](glm::vec3 v, size_t index) {
                        Vertex new_vertex = {
                            .position = v,
                            .uv_x = 0,
                            .normal = { 1.0f, 0.0f, 0.0f },
                            .uv_y = 0,
                            .color = glm::vec4 { 1.0f }
                        };
                        vertices[initial_vertex + index] = new_vertex;
                });
            }

            // Load vertex normals
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second],
                [&](glm::vec3 v, size_t index) {
                    vertices[initial_vertex + index].normal = v;
                });
            }

            // Load UVs
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*uv).second],
                [&](glm::vec2 v, size_t index) {
                    vertices[initial_vertex + index].uv_x = v.x;
                    vertices[initial_vertex + index].uv_y = v.y;
                });
            }

            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second],
                [&](glm::vec4 v, size_t index) {
                    vertices[initial_vertex + index].color = v;
                });
            }

            mesh_asset.surfaces.push_back(surface);
        }

        // Display vertex normals for testing purposes
        if (OVERRIDE_COLORS) {
            for(Vertex& vtx : vertices) {
                vtx.color = glm::vec4(vtx.normal, 1.0f);
            }
        }

        mesh_asset.mesh_buffers = vk_engine->UploadMesh(vertices, indices);
        meshes.emplace_back(std::make_shared<MeshAsset>(std::move(mesh_asset)));
    }

    return meshes;
}


VertexInputDescription Vertex::get_vertex_description() {
    VertexInputDescription description;

    VkVertexInputBindingDescription main_binding = {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    description.bindings.push_back(main_binding);

    VkVertexInputAttributeDescription attrib_position = {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, position)
    };
    description.attributes.push_back(attrib_position);
    
    VkVertexInputAttributeDescription attrib_normal = {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, normal)
    };
    description.attributes.push_back(attrib_normal);

    VkVertexInputAttributeDescription attrib_color = {
        .location = 2,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, color)
    };
    description.attributes.push_back(attrib_color);

    return description;
}