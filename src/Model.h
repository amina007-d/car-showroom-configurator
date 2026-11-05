// Model.h - loads a Wavefront OBJ + MTL with tinyobjloader.
// Every OBJ object ("o name") becomes a ModelObject; inside it, faces are
// split by material into separate Meshes (one draw call per material).
// Objects whose name starts with "wheel" are re-centred on their bounding-box
// centre, so they can rotate around their own axle (correct pivot, R3).
#pragma once

#include "Mesh.h"
#include "SceneNode.h"
#include "Texture.h"

#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct ModelPart {
    std::string materialName;
    std::unique_ptr<Mesh> mesh;
    Material material;
};

struct ModelObject {
    std::string name;
    glm::vec3 pivot{0.0f};          // where the object's local origin sits in model space
    std::vector<ModelPart> parts;
};

class Model {
public:
    // Returns nullptr (and prints why) if the file cannot be loaded.
    static std::unique_ptr<Model> load(const std::string& objPath);

    std::vector<ModelObject> objects;
    size_t triangleCount = 0;

private:
    std::map<std::string, std::unique_ptr<Texture>> textures_;   // shared by all parts
    const Texture* texture(const std::string& path);
};
