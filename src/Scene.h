#pragma once

#include "Cubemap.h"
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

struct SpotLight {
    std::string name;
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;
    float innerDeg;
    float outerDeg;
};

struct Car {
    std::string name;
    std::unique_ptr<Model> model;
    SceneNode* root = nullptr;
    Material* paint = nullptr;
    std::array<SceneNode*, 4> wheels{};
};

struct PaintPreset {
    const char* name;
    glm::vec3 color;
    float shininess;
    float specular;
    float reflectivity;
};

class Scene {
public:
    static constexpr int MAX_LIGHTS = 4;
    static constexpr int MAX_SPOTS = 4;

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
    std::vector<SpotLight> spots;
    bool spotlightsOn = true;

    int environment = 0;
    void applyEnvironment(int index);
    const Cubemap& environmentMap() const { return *environmentMaps_[environment]; }

    Material* paint = nullptr;
    static const std::array<PaintPreset, 4> PAINTS;
    void applyPaint(const PaintPreset& p);

    std::vector<Car> cars;
    int selectedCar = 0;
    void selectCar(int index);

private:
    std::unique_ptr<Mesh> cube_, plane_, quad_, cylinder_, sphere_;
    std::unique_ptr<Texture> floorTex_;
    std::unique_ptr<SceneNode> root_;
    std::vector<std::unique_ptr<Cubemap>> environmentMaps_;
    std::vector<SceneNode*> walls_;

    SceneNode* turntable_ = nullptr;
    std::array<SceneNode*, 4> placeholderWheels_{};
    float keyLightAngle_ = 0.0f;

    SceneNode* addBox(SceneNode* parent, const std::string& name, glm::vec3 pos,
                      glm::vec3 size, const Material& mat);
    SceneNode* addQuad(SceneNode* parent, const std::string& name, glm::vec3 pos,
                       glm::vec3 rotation, glm::vec2 size, const Material& mat);
    void buildStudio();
    void buildRoom();
    void buildCars(SceneNode* turntable);
    bool addCar(SceneNode* turntable, const std::string& name, const std::string& objPath, int paintPreset);
    void buildPlaceholderCar(SceneNode* parent);
    void buildLights();
    void buildSpotlights();
    void buildEnvironmentMaps();
};
