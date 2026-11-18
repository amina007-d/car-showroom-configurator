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
};

inline const std::array<Environment, 3> ENVIRONMENTS = {{
    {"Studio", "",
     {0.05f, 0.05f, 0.06f}, {0.12f, 0.12f, 0.13f}, {0.02f, 0.02f, 0.02f}, 0.08f,
     {{{{1.0f, 0.95f, 0.85f}, 1.2f}, {{0.6f, 0.7f, 1.0f}, 0.6f}, {{1.0f, 1.0f, 1.0f}, 0.9f}}}, true},
    {"City", "assets/skybox/city",
     {0.35f, 0.45f, 0.6f}, {0.6f, 0.6f, 0.6f}, {0.1f, 0.1f, 0.1f}, 0.12f,
     {{{{1.0f, 0.97f, 0.9f}, 1.0f}, {{0.7f, 0.8f, 1.0f}, 0.8f}, {{1.0f, 0.9f, 0.8f}, 0.7f}}}, false},
    {"Night", "assets/skybox/night",
     {0.01f, 0.01f, 0.03f}, {0.05f, 0.04f, 0.08f}, {0.01f, 0.01f, 0.01f}, 0.04f,
     {{{{0.6f, 0.7f, 1.0f}, 0.7f}, {{0.3f, 0.35f, 0.6f}, 0.4f}, {{1.0f, 0.6f, 0.3f}, 0.8f}}}, false},
}};
