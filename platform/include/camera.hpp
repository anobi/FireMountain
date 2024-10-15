#pragma once

#include <glm/glm.hpp>


struct Camera {
    void Update();
    glm::vec3 velocity;
    glm::vec3 position;

    float pitch { 0.0f };
    float yaw { 0.0f };

    glm::mat4 get_view_matrix();
    glm::mat4 get_rotation_matrix();
};