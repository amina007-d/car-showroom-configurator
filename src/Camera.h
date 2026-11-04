// Camera.h - orbit camera that circles around a target point.
// Position is computed from spherical coordinates (yaw, pitch, distance).
#pragma once

#include <glm/glm.hpp>

class Camera {
public:
    glm::vec3 target{0.0f, 0.6f, 0.0f};
    float yaw = 35.0f;        // degrees, around the Y axis
    float pitch = 18.0f;      // degrees, above the horizon
    float distance = 7.0f;

    float minDistance = 3.0f, maxDistance = 15.0f;
    float minPitch = 2.0f, maxPitch = 80.0f;

    glm::vec3 position() const;
    glm::mat4 view() const;
    glm::mat4 projection(float aspect) const;

    // All deltas are already scaled by dt (keyboard) or by pixels (mouse),
    // so movement speed does not depend on the frame rate.
    void rotate(float dYaw, float dPitch);
    void zoom(float dDistance);
    void reset();
};
