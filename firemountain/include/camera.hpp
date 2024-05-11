#pragma once

#include <vk_types.hpp>

class Camera {
public:

    void Update();

    // TODO: Remove the one above and use this to recalculate the matrices
    // void Update(float pitch, float yaw, glm::vec3 position);

    // TODO: Remove these
    glm::vec3 velocity;
    glm::vec3 position;

    float pitch { 0.0f };
    float yaw { 0.0f };

    // TODO: and just have the camera hold these?
//private:
    glm::mat4 get_view_matrix();
    glm::mat4 get_rotation_matrix();
};