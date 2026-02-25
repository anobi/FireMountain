#pragma once

#include <EASTL/vector.h>
#include <glm/vec3.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
};

struct LoaderMesh {
    eastl::vector<Vertex> vertices;
    bool load_from_obj(const char* path);
};
