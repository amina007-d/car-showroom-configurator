#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <iostream>
#include <vector>

Texture::~Texture() {
    if (id_) glDeleteTextures(1, &id_);
}

void Texture::upload(int w, int h, int channels, const unsigned char* data, bool srgb) {
    GLenum format = channels == 4 ? GL_RGBA : (channels == 3 ? GL_RGB : GL_RED);
    GLenum internal = format;
    if (srgb && channels == 4) internal = GL_SRGB8_ALPHA8;
    if (srgb && channels == 3) internal = GL_SRGB8;
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internal, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);                                    // R5: mipmapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

std::unique_ptr<Texture> Texture::fromFile(const std::string& path, bool srgb) {
    int w, h, ch;
    stbi_set_flip_vertically_on_load(true);   // OpenGL expects (0,0) at the bottom-left
    // Always ask for 4 channels: grey+alpha or RGB images are expanded to RGBA,
    // so every texture has an alpha channel (used by glass, lights and the shadow).
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 4);
    if (!data) {
        std::cerr << "[Texture] could not load " << path << ", using fallback\n";
        return nullptr;
    }
    std::unique_ptr<Texture> t(new Texture());
    t->upload(w, h, 4, data, srgb);
    stbi_image_free(data);
    return t;
}

std::unique_ptr<Texture> Texture::checker(int size, int cells, glm::vec3 a, glm::vec3 b) {
    std::vector<unsigned char> pixels(size * size * 3);
    int cell = size / cells;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            glm::vec3 c = ((x / cell + y / cell) % 2 == 0) ? a : b;
            unsigned char* p = &pixels[(y * size + x) * 3];
            p[0] = static_cast<unsigned char>(c.r * 255);
            p[1] = static_cast<unsigned char>(c.g * 255);
            p[2] = static_cast<unsigned char>(c.b * 255);
        }
    }
    std::unique_ptr<Texture> t(new Texture());
    t->upload(size, size, 3, pixels.data(), false);
    return t;
}

void Texture::bind(int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id_);
}
