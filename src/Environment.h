#pragma once

#include <glm/glm.hpp>
#include <array>
#include <string>

struct LightPreset {
    glm::vec3 color;
    float intensity;
};

struct Environment {
    std::string name;
    std::string skyboxFolder;
    glm::vec3 skyTop, skyHorizon, skyBottom;
    float ambient;
    std::array<LightPreset, 3> lights;
    bool walls;
    glm::vec3 wallColor;
    glm::vec3 ceilingColor;
    float floorTint;
    float floorReflectivity;
};

inline const std::array<Environment, 3> ENVIRONMENTS = {{
    {"Studio", "",
     {0.05f, 0.05f, 0.06f}, {0.12f, 0.12f, 0.13f}, {0.02f, 0.02f, 0.02f}, 0.06f,
     {{{{1.0f, 0.95f, 0.85f}, 0.8f}, {{0.6f, 0.7f, 1.0f}, 0.35f}, {{1.0f, 1.0f, 1.0f}, 0.7f}}}, true,
     {0.022f, 0.022f, 0.025f}, {0.012f, 0.012f, 0.014f}, 0.055f, 0.08f},
    {"Day", "assets/skybox/city",
     {0.30f, 0.50f, 0.82f}, {0.78f, 0.84f, 0.90f}, {0.30f, 0.30f, 0.31f}, 0.28f,
     {{{{1.0f, 0.98f, 0.94f}, 1.3f}, {{0.85f, 0.9f, 1.0f}, 1.0f}, {{1.0f, 1.0f, 1.0f}, 0.9f}}}, false,
     {0.55f, 0.55f, 0.53f}, {0.70f, 0.70f, 0.70f}, 0.70f, 0.10f},
    {"Night", "assets/skybox/night",
     {0.01f, 0.015f, 0.05f}, {0.10f, 0.08f, 0.16f}, {0.02f, 0.02f, 0.025f}, 0.05f,
     {{{{1.0f, 0.88f, 0.70f}, 0.7f}, {{0.35f, 0.40f, 0.70f}, 0.35f}, {{0.6f, 0.65f, 0.9f}, 0.6f}}}, false,
     {0.10f, 0.10f, 0.11f}, {0.08f, 0.08f, 0.09f}, 0.30f, 0.12f},
}};
