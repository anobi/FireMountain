#include <camera.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

void Camera::Update() {
    float speed_multiplier = 0.05f;
    auto rotation = get_rotation_matrix();
    this->position += glm::vec3(rotation * glm::vec4(this->velocity * speed_multiplier, 0.0f));
}

glm::mat4 Camera::GetViewProjectionMatrix(float screen_width, float screen_height) {
    auto v = this->get_view_matrix();
    auto p = this->GetPerspectiveProjection(screen_width, screen_height);
    return p * v;
}

glm::mat4 Camera::GetPerspectiveProjection(float screen_width, float screen_height) {
    auto projection = glm::perspective(
        glm::radians(70.0f), 
        screen_width / screen_height,
        10000.0f,
        0.1f
    );
    projection[1][1] *= -1;  // Invert the Y axis to get into the gl land
    return projection;
}

glm::mat4 Camera::get_view_matrix() {
    auto translation = glm::translate(glm::mat4(1.0f), position);
    auto rotation = get_rotation_matrix();
    return glm::inverse(translation * rotation);
}

glm::mat4 Camera::get_rotation_matrix() {
    auto pitch_q = glm::angleAxis(this->pitch, glm::vec3 { 1.0f, 0.0f, 0.0f });
    auto yaw_q = glm::angleAxis(this->yaw, glm::vec3 { 0.0f, -1.0f, 0.0f });
    return glm::toMat4(yaw_q) * glm::toMat4(pitch_q);
}