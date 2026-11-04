// SceneNode.h - node of the scene graph (R3).
// Each node has a local transform (translate * rotate * scale) relative to its
// parent, an optional mesh + material, and any number of children.
// World matrix of a node = parent world matrix * local matrix.
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
    float shininess = 32.0f;     // Blinn-Phong exponent: low = matte, high = glossy
    float specular = 0.5f;       // strength of the specular term
    glm::vec3 emissive{0.0f};    // self-illumination (lamps, headlights)
    const Texture* texture = nullptr;
    float alpha = 1.0f;          // opacity (multiplied by the texture alpha)
    bool blend = false;          // true -> drawn in the transparent pass (glass, lights)
    bool doubleSided = false;    // true -> back-face culling is off for this mesh
};

class SceneNode {
public:
    explicit SceneNode(std::string name) : name(std::move(name)) {}

    std::string name;
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};    // Euler angles in degrees, applied X then Y then Z
    glm::vec3 scale{1.0f};
    const Mesh* mesh = nullptr;  // nullptr = pure group node (no geometry)
    Material material;

    SceneNode* addChild(std::unique_ptr<SceneNode> child);
    glm::mat4 localMatrix() const;

    // Recursively draws this node and all children. Only meshes whose
    // material.blend equals transparentPass are drawn, so the scene is drawn
    // twice: opaque objects first, then transparent ones with blending on.
    void draw(const Shader& shader, const glm::mat4& parentWorld, bool transparentPass) const;

private:
    std::vector<std::unique_ptr<SceneNode>> children_;
};
