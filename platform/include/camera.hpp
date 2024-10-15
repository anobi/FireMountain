#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

struct Camera {
    void Update();
    glm::vec3 velocity;
    glm::vec3 position;

    float pitch { 0.0f };
    float yaw { 0.0f };

    glm::mat4 get_view_matrix();
    glm::mat4 get_rotation_matrix();
};