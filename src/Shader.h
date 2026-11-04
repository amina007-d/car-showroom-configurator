// Shader.h - loads a vertex + fragment shader pair from .vert/.frag files,
// links them into a program and provides typed uniform setters.
#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
    Shader(const std::string& vertPath, const std::string& fragPath);
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const { glUseProgram(id_); }
    GLuint id() const { return id_; }

    void setBool (const std::string& name, bool v) const;
    void setInt  (const std::string& name, int v) const;
    void setFloat(const std::string& name, float v) const;
    void setVec3 (const std::string& name, const glm::vec3& v) const;
    void setMat3 (const std::string& name, const glm::mat3& m) const;
    void setMat4 (const std::string& name, const glm::mat4& m) const;

private:
    GLuint id_ = 0;
    GLint location(const std::string& name) const;
    static std::string readFile(const std::string& path);
    static GLuint compile(GLenum type, const std::string& src, const std::string& path);
};
