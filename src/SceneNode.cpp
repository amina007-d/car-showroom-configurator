#include "SceneNode.h"

#include <glm/gtc/matrix_transform.hpp>

SceneNode* SceneNode::addChild(std::unique_ptr<SceneNode> child) {
    children_.push_back(std::move(child));
    return children_.back().get();
}

glm::mat4 SceneNode::localMatrix() const {
    glm::mat4 m(1.0f);
    m = glm::translate(m, position);
    m = glm::rotate(m, glm::radians(rotation.z), glm::vec3(0, 0, 1));
    m = glm::rotate(m, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    m = glm::rotate(m, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    m = glm::scale(m, scale);
    return m;   // = T * Rz * Ry * Rx * S
}

void SceneNode::draw(const Shader& shader, const glm::mat4& parentWorld, bool transparentPass) const {
    glm::mat4 world = parentWorld * localMatrix();

    if (mesh && material.blend == transparentPass) {
        shader.setMat4("uModel", world);
        // Normal matrix keeps normals perpendicular under non-uniform scaling.
        shader.setMat3("uNormalMatrix", glm::transpose(glm::inverse(glm::mat3(world))));
        shader.setVec3("uMaterial.color", material.color);
        shader.setFloat("uMaterial.shininess", material.shininess);
        shader.setFloat("uMaterial.specular", material.specular);
        shader.setVec3("uMaterial.emissive", material.emissive);
        shader.setFloat("uMaterial.alpha", material.alpha);
        shader.setBool("uMaterial.useTexture", material.texture != nullptr);
        if (material.texture) material.texture->bind(0);
        if (material.doubleSided) glDisable(GL_CULL_FACE);
        else                      glEnable(GL_CULL_FACE);
        mesh->draw();
    }

    for (const auto& child : children_) child->draw(shader, world, transparentPass);
}
