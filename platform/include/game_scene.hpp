#pragma once

#include <string>
#include <glm/glm.hpp>

struct GameSceneObject {
    std::string name;
    std::string mesh_file;
    glm::vec3 location;
    glm::vec3 rotation;
};