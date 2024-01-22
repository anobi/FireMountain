#include <tiny_obj_loader.h>
#include <fmt/core.h>

#include "fm_mesh_loader.hpp"


const int FACE_VERTICES = 3;


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
                    .color = {normal_x, normal_y, normal_z}
                };

                vertices->push_back(vertex);
            }
            index_offset += FACE_VERTICES;
        }
    }
    return true;
}

bool MeshLoader::LoadGltf(const char *path, std::span<Vertex> *vertices, std::span<uint32_t> *indices)
{
    return false;
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