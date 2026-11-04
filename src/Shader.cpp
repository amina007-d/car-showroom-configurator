#include "Shader.h"

#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

std::string Shader::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot open shader file: " + path +
                                        " (run the program from the project root)");
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint Shader::compile(GLenum type, const std::string& src, const std::string& path) {
    GLuint s = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(s, 1, &c, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        throw std::runtime_error("Shader compile error in " + path + ":\n" + log);
    }
    return s;
}

Shader::Shader(const std::string& vertPath, const std::string& fragPath) {
    GLuint vs = compile(GL_VERTEX_SHADER, readFile(vertPath), vertPath);
    GLuint fs = compile(GL_FRAGMENT_SHADER, readFile(fragPath), fragPath);
    id_ = glCreateProgram();
    glAttachShader(id_, vs);
    glAttachShader(id_, fs);
    glLinkProgram(id_);
    GLint ok = 0;
    glGetProgramiv(id_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(id_, sizeof(log), nullptr, log);
        throw std::runtime_error("Shader link error (" + vertPath + ", " + fragPath + "):\n" + log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
}

Shader::~Shader() {
    if (id_) glDeleteProgram(id_);
}

GLint Shader::location(const std::string& name) const {
    return glGetUniformLocation(id_, name.c_str());
}

void Shader::setBool (const std::string& n, bool v) const { glUniform1i(location(n), v ? 1 : 0); }
void Shader::setInt  (const std::string& n, int v) const { glUniform1i(location(n), v); }
void Shader::setFloat(const std::string& n, float v) const { glUniform1f(location(n), v); }
void Shader::setVec3 (const std::string& n, const glm::vec3& v) const { glUniform3fv(location(n), 1, glm::value_ptr(v)); }
void Shader::setMat3 (const std::string& n, const glm::mat3& m) const { glUniformMatrix3fv(location(n), 1, GL_FALSE, glm::value_ptr(m)); }
void Shader::setMat4 (const std::string& n, const glm::mat4& m) const { glUniformMatrix4fv(location(n), 1, GL_FALSE, glm::value_ptr(m)); }
