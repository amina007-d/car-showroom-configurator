#pragma once

#include "Environment.h"
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
    SceneNode* marker = nullptr;
};

struct PaintPreset {
    const char* name;
    glm::vec3 color;
    float shininess;
    float specular;
};

class Scene {
public:
    static constexpr int MAX_LIGHTS = 4;

    void build();
    void update(float dt);
    void draw(const Shader& shader, bool transparentPass) const;
    void uploadLights(const Shader& shader) const;

    float turntableSpeed = 20.0f;
    bool turntablePaused = false;
    bool keyLightOrbits = true;
    float wheelSpin = 0.0f;
    float ambient = 0.08f;
    std::vector<PointLight> lights;

    int environment = 0;
    void applyEnvironment(int index);

    Material* paint = nullptr;
    static const std::array<PaintPreset, 4> PAINTS;
    void applyPaint(const PaintPreset& p);

private:
    std::unique_ptr<Mesh> cube_, plane_, cylinder_, sphere_;
    std::unique_ptr<Texture> floorTex_;
    std::unique_ptr<SceneNode> root_;
    std::unique_ptr<Model> carModel_;

    SceneNode* turntable_ = nullptr;
    std::array<SceneNode*, 4> wheels_{};
    float keyLightAngle_ = 0.0f;

    SceneNode* addBox(SceneNode* parent, const std::string& name, glm::vec3 pos,
                      glm::vec3 size, const Material& mat);
    void buildStudio();
    void buildCar(SceneNode* parent);
    void buildPlaceholderCar(SceneNode* parent);
    void buildLights();
};
