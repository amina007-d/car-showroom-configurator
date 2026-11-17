#pragma once

#include "Cubemap.h"
#include "Shader.h"

#include <glm/glm.hpp>
#include <memory>

class Skybox {
public:
    Skybox();
    ~Skybox();
    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;

    void draw(const Cubemap& cubemap, const glm::mat4& view, const glm::mat4& projection) const;

private:
    std::unique_ptr<Shader> shader_;
    GLuint vao_ = 0, vbo_ = 0;
};
