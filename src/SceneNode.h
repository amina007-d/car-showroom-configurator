#pragma once

#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

struct Material {
    glm::vec3 color{1.0f};
    float shininess = 32.0f;
    float specular = 0.5f;
    glm::vec3 emissive{0.0f};
    const Texture* texture = nullptr;
    float alpha = 1.0f;
    bool blend = false;
    bool doubleSided = false;
};

class SceneNode {
public:
    explicit SceneNode(std::string name) : name(std::move(name)) {}

    std::string name;
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};
    const Mesh* mesh = nullptr;
    Material material;
    bool visible = true;

    SceneNode* addChild(std::unique_ptr<SceneNode> child);
    glm::mat4 localMatrix() const;

    void draw(const Shader& shader, const glm::mat4& parentWorld, bool transparentPass) const;

private:
    std::vector<std::unique_ptr<SceneNode>> children_;
};
