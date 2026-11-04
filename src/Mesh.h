// Mesh.h - geometry stored on the GPU (VAO + VBO + EBO) and factory
// functions for the primitives used in the scene.
#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void draw() const;

    // Primitive factories. All are centred at the origin.
    static std::unique_ptr<Mesh> cube();                                   // 1 x 1 x 1
    static std::unique_ptr<Mesh> plane(float size, float uvRepeat);        // XZ plane, normal +Y
    static std::unique_ptr<Mesh> cylinder(int segments);                   // radius 0.5, height 1, axis Y
    static std::unique_ptr<Mesh> sphere(int stacks, int slices);           // radius 0.5

private:
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
    GLsizei indexCount_ = 0;
};
