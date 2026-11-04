// Scene.h - the showroom: studio, turntable, car and studio lights.
// Builds the scene graph once and animates it every frame.
#pragma once

#include "Mesh.h"
#include "Model.h"
#include "SceneNode.h"
#include "Shader.h"
#include "Texture.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

struct PointLight {
    std::string name;
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    bool enabled = true;
    SceneNode* marker = nullptr;   // small glowing sphere that shows where the light is
};

struct PaintPreset {
    const char* name;
    glm::vec3 color;
    float shininess;
    float specular;
};

class Scene {
public:
    static constexpr int MAX_LIGHTS = 4;   // must match MAX_LIGHTS in phong.frag

    void build();
    void update(float dt);
    void draw(const Shader& shader, bool transparentPass) const;
    void uploadLights(const Shader& shader) const;

    // --- state changed by the UI / keyboard ---
    float turntableSpeed = 20.0f;      // degrees per second
    bool turntablePaused = false;
    bool keyLightOrbits = true;        // R4: the key light moves around the car
    float wheelSpin = 0.0f;            // degrees per second, 0 = still
    float ambient = 0.08f;
    std::vector<PointLight> lights;

    Material* paint = nullptr;         // material of the car body (changed by the configurator)
    static const std::array<PaintPreset, 4> PAINTS;
    void applyPaint(const PaintPreset& p);

private:
    std::unique_ptr<Mesh> cube_, plane_, cylinder_, sphere_;
    std::unique_ptr<Texture> floorTex_;
    std::unique_ptr<SceneNode> root_;
    std::unique_ptr<Model> carModel_;   // must outlive the nodes that point to its meshes

    SceneNode* turntable_ = nullptr;
    std::array<SceneNode*, 4> wheels_{};
    float keyLightAngle_ = 0.0f;

    SceneNode* addBox(SceneNode* parent, const std::string& name, glm::vec3 pos,
                      glm::vec3 size, const Material& mat);
    void buildStudio();
    void buildCar(SceneNode* parent);              // OBJ model, or the placeholder if it is missing
    void buildPlaceholderCar(SceneNode* parent);
    void buildLights();
};
