#include "Model.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

#include <cstdint>
#include <iostream>
#include <unordered_map>

const Texture* Model::texture(const std::string& path) {
    auto it = textures_.find(path);
    if (it != textures_.end()) return it->second.get();
    auto tex = Texture::fromFile(path, true);
    const Texture* raw = tex.get();
    textures_[path] = std::move(tex);       // nullptr is cached too, so we only try once
    return raw;
}

static Material toMaterial(const tinyobj::material_t& m) {
    Material mat;
    mat.color = {m.diffuse[0], m.diffuse[1], m.diffuse[2]};
    mat.shininess = m.shininess > 0 ? m.shininess : 32.0f;
    mat.specular = m.specular[0];
    mat.alpha = m.dissolve;
    auto flag = [&](const char* key) {
        auto it = m.unknown_parameter.find(key);
        return it != m.unknown_parameter.end() && it->second == "1";
    };
    mat.blend = flag("blend") || m.dissolve < 1.0f;
    mat.doubleSided = flag("double_sided");
    return mat;
}

std::unique_ptr<Model> Model::load(const std::string& objPath) {
    std::string dir = objPath.substr(0, objPath.find_last_of('/') + 1);

    tinyobj::ObjReaderConfig config;
    config.triangulate = true;
    config.mtl_search_path = dir;
    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(objPath, config)) {
        std::cerr << "[Model] cannot load " << objPath << ": " << reader.Error() << "\n";
        return nullptr;
    }

    const auto& attrib = reader.GetAttrib();
    const auto& materials = reader.GetMaterials();
    auto model = std::unique_ptr<Model>(new Model());

    for (const auto& shape : reader.GetShapes()) {
        // Group this shape's triangles by material id.
        struct Bucket {
            std::vector<Vertex> vertices;
            std::vector<unsigned int> indices;
            std::unordered_map<uint64_t, unsigned int> lookup;   // (v, vt, vn) -> vertex index
        };
        std::map<int, Bucket> buckets;

        const auto& idx = shape.mesh.indices;
        for (size_t f = 0; f < shape.mesh.material_ids.size(); ++f) {
            Bucket& b = buckets[shape.mesh.material_ids[f]];
            for (int k = 0; k < 3; ++k) {
                const tinyobj::index_t& i = idx[3 * f + k];
                // Pack the three indices into one key so identical corners share a vertex.
                uint64_t key = (uint64_t(uint32_t(i.vertex_index)) << 42) ^
                               (uint64_t(uint32_t(i.texcoord_index) & 0x1FFFFF) << 21) ^
                               (uint64_t(uint32_t(i.normal_index) & 0x1FFFFF));
                auto found = b.lookup.find(key);
                if (found != b.lookup.end()) {
                    b.indices.push_back(found->second);
                    continue;
                }
                Vertex v{};
                v.position = {attrib.vertices[3 * i.vertex_index + 0],
                              attrib.vertices[3 * i.vertex_index + 1],
                              attrib.vertices[3 * i.vertex_index + 2]};
                if (i.normal_index >= 0)
                    v.normal = {attrib.normals[3 * i.normal_index + 0],
                                attrib.normals[3 * i.normal_index + 1],
                                attrib.normals[3 * i.normal_index + 2]};
                if (i.texcoord_index >= 0)
                    v.uv = {attrib.texcoords[2 * i.texcoord_index + 0],
                            attrib.texcoords[2 * i.texcoord_index + 1]};
                unsigned int newIndex = static_cast<unsigned int>(b.vertices.size());
                b.vertices.push_back(v);
                b.lookup[key] = newIndex;
                b.indices.push_back(newIndex);
            }
        }

        ModelObject obj;
        obj.name = shape.name;

        // Wheels: move the pivot to the bounding-box centre (the axle).
        if (obj.name.rfind("wheel", 0) == 0) {
            glm::vec3 lo(1e9f), hi(-1e9f);
            for (auto& [id, b] : buckets)
                for (auto& v : b.vertices) { lo = glm::min(lo, v.position); hi = glm::max(hi, v.position); }
            obj.pivot = (lo + hi) * 0.5f;
            for (auto& [id, b] : buckets)
                for (auto& v : b.vertices) v.position -= obj.pivot;
        }

        for (auto& [id, b] : buckets) {
            ModelPart part;
            if (id >= 0 && id < static_cast<int>(materials.size())) {
                const auto& m = materials[id];
                part.materialName = m.name;
                part.material = toMaterial(m);
                if (!m.diffuse_texname.empty()) part.material.texture = model->texture(dir + m.diffuse_texname);
            }
            model->triangleCount += b.indices.size() / 3;
            part.mesh = std::make_unique<Mesh>(b.vertices, b.indices);
            obj.parts.push_back(std::move(part));
        }
        model->objects.push_back(std::move(obj));
    }

    std::cout << "[Model] " << objPath << ": " << model->objects.size() << " objects, "
              << model->triangleCount << " triangles\n";
    return model;
}
