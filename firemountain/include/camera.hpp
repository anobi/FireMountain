#pragma once

#include <vk_types.hpp>

class Camera {
public:

    void Update();

    glm::vec3 velocity;
    glm::vec3 position;

    float pitch { 0.0f };
    float yaw { 0.0f };

//private:
    glm::mat4 get_view_matrix();
    glm::mat4 get_rotation_matrix();
};