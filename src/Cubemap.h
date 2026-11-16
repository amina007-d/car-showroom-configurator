#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>

class Cubemap {
public:
    ~Cubemap();
    Cubemap(const Cubemap&) = delete;
    Cubemap& operator=(const Cubemap&) = delete;

    static std::unique_ptr<Cubemap> fromFolder(const std::string& folder);
    static std::unique_ptr<Cubemap> gradient(int size, glm::vec3 top, glm::vec3 horizon, glm::vec3 bottom);

    void bind(int unit) const;

private:
    Cubemap() = default;
    GLuint id_ = 0;
    void begin();
    void end();
};
