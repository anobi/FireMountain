#include <tiny_obj_loader.h>
#include <iostream>

#include "mesh.hpp"

const int FACE_VERTICES = 3;

bool Mesh::load_from_obj(const char* path) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warnings;
    std::string errors;
    tinyobj::LoadObj(&attrib, &shapes, &materials, &warnings, &errors, path, nullptr);
    if(!warnings.empty()) {
        std::cout << "WARNING: " << warnings << std::endl;
    }
    if(!errors.empty()) {
        std::cerr << errors << std::endl;
        return false;
    }

    for (size_t s = 0; s < shapes.size(); s++) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            for (size_t v = 0; v < FACE_VERTICES; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

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

                this->vertices.push_back(vertex);
            }
            index_offset += FACE_VERTICES;
        }
    }
    return true;
}