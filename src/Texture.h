// Texture.h - 2D texture with mipmaps. Loads an image with stb_image or,
// if the file is missing, builds a procedural checkerboard instead.
#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>

class Texture {
public:
    ~Texture();
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Returns nullptr if the file cannot be loaded.
    // srgb = true for colour images: the GPU converts them to linear space when sampling,
    // so lighting is computed in linear space like the rest of the shader.
    static std::unique_ptr<Texture> fromFile(const std::string& path, bool srgb = true);
    static std::unique_ptr<Texture> checker(int size, int cells, glm::vec3 a, glm::vec3 b);

    void bind(int unit) const;

private:
    Texture() = default;
    GLuint id_ = 0;
    void upload(int w, int h, int channels, const unsigned char* data, bool srgb);
};
