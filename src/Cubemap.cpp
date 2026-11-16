#include "Cubemap.h"

#include <stb/stb_image.h>

#include <cmath>
#include <iostream>
#include <vector>

static const char* FACES[6] = {"px", "nx", "py", "ny", "pz", "nz"};

Cubemap::~Cubemap() {
    if (id_) glDeleteTextures(1, &id_);
}

void Cubemap::begin() {
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

void Cubemap::end() {
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

std::unique_ptr<Cubemap> Cubemap::fromFolder(const std::string& folder) {
    std::unique_ptr<Cubemap> cube(new Cubemap());
    cube->begin();
    stbi_set_flip_vertically_on_load(false);
    for (int i = 0; i < 6; ++i) {
        int w = 0, h = 0, ch = 0;
        unsigned char* data = nullptr;
        for (const char* ext : {".png", ".jpg"}) {
            data = stbi_load((folder + "/" + FACES[i] + ext).c_str(), &w, &h, &ch, 3);
            if (data) break;
        }
        if (!data) {
            std::cerr << "[Cubemap] missing face " << FACES[i] << " in " << folder << "\n";
            stbi_set_flip_vertically_on_load(true);
            return nullptr;
        }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }
    stbi_set_flip_vertically_on_load(true);
    cube->end();
    return cube;
}

static glm::vec3 faceDirection(int face, float s, float t) {
    switch (face) {
        case 0:  return { 1.0f, -t, -s};
        case 1:  return {-1.0f, -t,  s};
        case 2:  return { s,  1.0f,  t};
        case 3:  return { s, -1.0f, -t};
        case 4:  return { s, -t,  1.0f};
        default: return {-s, -t, -1.0f};
    }
}

std::unique_ptr<Cubemap> Cubemap::gradient(int size, glm::vec3 top, glm::vec3 horizon, glm::vec3 bottom) {
    std::unique_ptr<Cubemap> cube(new Cubemap());
    cube->begin();
    std::vector<unsigned char> pixels(size * size * 3);
    for (int face = 0; face < 6; ++face) {
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                float s = 2.0f * (x + 0.5f) / size - 1.0f;
                float t = 2.0f * (y + 0.5f) / size - 1.0f;
                float up = glm::normalize(faceDirection(face, s, t)).y;
                glm::vec3 c = up >= 0.0f ? glm::mix(horizon, top, std::sqrt(up))
                                         : glm::mix(horizon, bottom, std::sqrt(-up));
                unsigned char* p = &pixels[(y * size + x) * 3];
                for (int k = 0; k < 3; ++k)
                    p[k] = static_cast<unsigned char>(255.0f * std::pow(glm::clamp(c[k], 0.0f, 1.0f), 1.0f / 2.2f));
            }
        }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_SRGB8, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    }
    cube->end();
    return cube;
}

void Cubemap::bind(int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id_);
}
