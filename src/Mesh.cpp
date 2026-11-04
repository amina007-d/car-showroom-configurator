#include "Mesh.h"

#include <cmath>
#include <cstddef>

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
    : indexCount_(static_cast<GLsizei>(indices.size())) {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // layout(location = 0) position, 1 normal, 2 uv
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

    glBindVertexArray(0);
}

Mesh::~Mesh() {
    glDeleteBuffers(1, &ebo_);
    glDeleteBuffers(1, &vbo_);
    glDeleteVertexArrays(1, &vao_);
}

void Mesh::draw() const {
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

// ---------------------------------------------------------------------------
// Primitive factories
// ---------------------------------------------------------------------------

std::unique_ptr<Mesh> Mesh::cube() {
    // 6 faces x 4 vertices so every face has its own flat normal.
    const glm::vec3 normals[6] = {{0,0,1},{0,0,-1},{1,0,0},{-1,0,0},{0,1,0},{0,-1,0}};
    std::vector<Vertex> v;
    std::vector<unsigned int> idx;
    for (int f = 0; f < 6; ++f) {
        glm::vec3 n = normals[f];
        // two axes spanning the face
        glm::vec3 u = (std::abs(n.y) > 0.5f) ? glm::vec3(1,0,0) : glm::vec3(0,1,0);
        glm::vec3 w = glm::cross(u, n);   // chosen so that w x u = n -> CCW winding seen from outside
        unsigned int base = static_cast<unsigned int>(v.size());
        const float s[4][2] = {{-1,-1},{1,-1},{1,1},{-1,1}};
        for (auto& c : s) {
            glm::vec3 p = 0.5f * (n + c[0] * w + c[1] * u);
            v.push_back({p, n, {(c[0] + 1) * 0.5f, (c[1] + 1) * 0.5f}});
        }
        idx.insert(idx.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
    }
    return std::make_unique<Mesh>(v, idx);
}

std::unique_ptr<Mesh> Mesh::plane(float size, float uvRepeat) {
    float h = size * 0.5f;
    std::vector<Vertex> v = {
        {{-h, 0, -h}, {0, 1, 0}, {0, 0}},
        {{ h, 0, -h}, {0, 1, 0}, {uvRepeat, 0}},
        {{ h, 0,  h}, {0, 1, 0}, {uvRepeat, uvRepeat}},
        {{-h, 0,  h}, {0, 1, 0}, {0, uvRepeat}},
    };
    std::vector<unsigned int> idx = {0, 2, 1, 0, 3, 2};   // counter-clockwise seen from +Y
    return std::make_unique<Mesh>(v, idx);
}

std::unique_ptr<Mesh> Mesh::cylinder(int segments) {
    std::vector<Vertex> v;
    std::vector<unsigned int> idx;
    const float r = 0.5f, h = 0.5f;
    const float TWO_PI = 6.28318530718f;

    // Side: duplicated seam vertex so UVs wrap correctly.
    for (int i = 0; i <= segments; ++i) {
        float t = static_cast<float>(i) / segments;
        float a = t * TWO_PI;
        glm::vec3 n(std::cos(a), 0, std::sin(a));
        v.push_back({{r * n.x, -h, r * n.z}, n, {t, 0}});
        v.push_back({{r * n.x,  h, r * n.z}, n, {t, 1}});
    }
    for (int i = 0; i < segments; ++i) {
        unsigned int b = i * 2;
        idx.insert(idx.end(), {b, b + 1, b + 3, b, b + 3, b + 2});
    }

    // Caps (top y = +h, bottom y = -h).
    for (int cap = 0; cap < 2; ++cap) {
        float y = cap == 0 ? h : -h;
        glm::vec3 n(0, cap == 0 ? 1.0f : -1.0f, 0);
        unsigned int centre = static_cast<unsigned int>(v.size());
        v.push_back({{0, y, 0}, n, {0.5f, 0.5f}});
        for (int i = 0; i <= segments; ++i) {
            float a = static_cast<float>(i) / segments * TWO_PI;
            float c = std::cos(a), s = std::sin(a);
            v.push_back({{r * c, y, r * s}, n, {0.5f + 0.5f * c, 0.5f + 0.5f * s}});
        }
        for (int i = 0; i < segments; ++i) {
            unsigned int a = centre + 1 + i, b = a + 1;
            if (cap == 0) idx.insert(idx.end(), {centre, b, a});
            else          idx.insert(idx.end(), {centre, a, b});
        }
    }
    return std::make_unique<Mesh>(v, idx);
}

std::unique_ptr<Mesh> Mesh::sphere(int stacks, int slices) {
    std::vector<Vertex> v;
    std::vector<unsigned int> idx;
    const float PI = 3.14159265359f;
    for (int i = 0; i <= stacks; ++i) {
        float phi = PI * i / stacks;                 // 0 (top) .. PI (bottom)
        for (int j = 0; j <= slices; ++j) {
            float theta = 2 * PI * j / slices;
            glm::vec3 n(std::sin(phi) * std::cos(theta), std::cos(phi), std::sin(phi) * std::sin(theta));
            v.push_back({0.5f * n, n, {static_cast<float>(j) / slices, static_cast<float>(i) / stacks}});
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            unsigned int a = i * (slices + 1) + j, b = a + slices + 1;
            idx.insert(idx.end(), {a, a + 1, b, a + 1, b + 1, b});
        }
    }
    return std::make_unique<Mesh>(v, idx);
}
