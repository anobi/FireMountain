#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>


class Camera {
    float pitch { 0.0f };
    float yaw { 0.0f };
    glm::vec3 velocity;
    glm::vec3 position;
};