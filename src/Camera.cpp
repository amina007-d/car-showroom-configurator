#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

glm::vec3 Camera::position() const {
    float y = glm::radians(yaw), p = glm::radians(pitch);
    glm::vec3 offset(std::cos(p) * std::sin(y), std::sin(p), std::cos(p) * std::cos(y));
    return target + distance * offset;
}

glm::mat4 Camera::view() const {
    return glm::lookAt(position(), target, glm::vec3(0, 1, 0));
}

glm::mat4 Camera::projection(float aspect) const {
    return glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
}

void Camera::rotate(float dYaw, float dPitch) {
    yaw += dYaw;
    pitch = std::clamp(pitch + dPitch, minPitch, maxPitch);
}

void Camera::zoom(float dDistance) {
    distance = std::clamp(distance + dDistance, minDistance, maxDistance);
}

void Camera::reset() {
    *this = Camera();
}
