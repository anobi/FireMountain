#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

enum CameraProjectionType
{
    PERSPECTIVE,
    ORTHO
};

struct Camera {
    void Update();
    glm::mat4 GetViewProjectionMatrix(float screen_width, float screen_height, CameraProjectionType type);
    glm::mat4 GetProjectionMatrix(float screen_width, float screen_height, CameraProjectionType projection_type);
    glm::mat4 GetPerspectiveProjection(float screen_width, float screen_height);
    glm::mat4 GetOrthoProjection(float screen_width, float screen_height);
    glm::vec3 velocity;
    glm::vec3 position;

    float pitch { 0.0f };
    float yaw { 0.0f };

    glm::mat4 get_view_matrix();
    glm::mat4 get_rotation_matrix();
};