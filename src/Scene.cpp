#include "Scene.h"

#include <cmath>

// Paint presets for the configurator (colours are in linear space).
const std::array<PaintPreset, 4> Scene::PAINTS = {{
    {"Matte black",   {0.015f, 0.015f, 0.017f},   6.0f, 0.08f},
    {"Glossy red",    {0.55f, 0.02f, 0.02f},    128.0f, 1.00f},
    {"Pearl white",   {0.80f, 0.80f, 0.78f},     64.0f, 0.70f},
    {"Midnight blue", {0.02f, 0.05f, 0.20f},     96.0f, 0.90f},
}};

void Scene::applyPaint(const PaintPreset& p) {
    paint->color = p.color;
    paint->shininess = p.shininess;
    paint->specular = p.specular;
}

SceneNode* Scene::addBox(SceneNode* parent, const std::string& name, glm::vec3 pos,
                         glm::vec3 size, const Material& mat) {
    auto node = std::make_unique<SceneNode>(name);
    node->mesh = cube_.get();
    node->position = pos;
    node->scale = size;
    node->material = mat;
    return parent->addChild(std::move(node));
}

void Scene::build() {
    cube_ = Mesh::cube();
    plane_ = Mesh::plane(40.0f, 20.0f);
    cylinder_ = Mesh::cylinder(48);
    sphere_ = Mesh::sphere(16, 24);

    // Floor texture: assets/textures/floor.png if it exists, otherwise a procedural checker.
    floorTex_ = Texture::fromFile("assets/textures/floor.png");
    if (!floorTex_) floorTex_ = Texture::checker(256, 2, {0.10f, 0.10f, 0.11f}, {0.07f, 0.07f, 0.08f});

    root_ = std::make_unique<SceneNode>("root");
    buildStudio();
    buildLights();
}

void Scene::buildStudio() {
    // Floor (textured, R5)
    auto floor = std::make_unique<SceneNode>("floor");
    floor->mesh = plane_.get();
    floor->material = {{1.0f, 1.0f, 1.0f}, 32.0f, 0.25f, {0, 0, 0}, floorTex_.get()};
    root_->addChild(std::move(floor));

    // Walls
    Material wall{{0.06f, 0.06f, 0.07f}, 8.0f, 0.05f};
    addBox(root_.get(), "back wall", {0.0f, 4.0f, -9.0f}, {40.0f, 8.0f, 0.2f}, wall);
    addBox(root_.get(), "left wall", {-9.0f, 4.0f, 0.0f}, {0.2f, 8.0f, 40.0f}, wall);

    // Turntable: base + rotating disc. The car is a child of the disc (R3).
    auto base = std::make_unique<SceneNode>("turntable base");
    base->mesh = cylinder_.get();
    base->position = {0.0f, 0.05f, 0.0f};
    base->scale = {5.0f, 0.1f, 5.0f};
    base->material = {{0.05f, 0.05f, 0.06f}, 16.0f, 0.2f};
    root_->addChild(std::move(base));

    auto disc = std::make_unique<SceneNode>("turntable");
    disc->position = {0.0f, 0.1f, 0.0f};
    turntable_ = root_->addChild(std::move(disc));

    auto top = std::make_unique<SceneNode>("turntable top");
    top->mesh = cylinder_.get();
    top->position = {0.0f, 0.05f, 0.0f};
    top->scale = {4.6f, 0.1f, 4.6f};
    top->material = {{0.10f, 0.10f, 0.11f}, 64.0f, 0.6f};
    turntable_->addChild(std::move(top));

    // Ring of small lamps around the turntable (decoration)
    for (int i = 0; i < 12; ++i) {
        float a = i / 12.0f * 6.2831853f;
        auto lamp = std::make_unique<SceneNode>("floor lamp");
        lamp->mesh = cylinder_.get();
        lamp->position = {2.6f * std::cos(a), 0.11f, 2.6f * std::sin(a)};
        lamp->scale = {0.12f, 0.02f, 0.12f};
        lamp->material = {{0.1f, 0.1f, 0.1f}, 8.0f, 0.0f, {0.9f, 0.85f, 0.7f}};
        root_->addChild(std::move(lamp));
    }

    auto car = std::make_unique<SceneNode>("car");
    car->position = {0.0f, 0.1f, 0.0f};
    buildCar(turntable_->addChild(std::move(car)));
}

// Loads the car model. Hierarchy: car -> body / wheel_* / shadow -> one child per material.
void Scene::buildCar(SceneNode* car) {
    carModel_ = Model::load("assets/models/porsche930/car.obj");
    if (!carModel_) {
        buildPlaceholderCar(car);
        return;
    }
    int wheelCount = 0;
    for (const ModelObject& obj : carModel_->objects) {
        auto group = std::make_unique<SceneNode>(obj.name);
        group->position = obj.pivot;
        SceneNode* g = car->addChild(std::move(group));
        if (obj.name.rfind("wheel", 0) == 0 && wheelCount < 4) wheels_[wheelCount++] = g;

        for (const ModelPart& part : obj.parts) {
            auto node = std::make_unique<SceneNode>(obj.name + "/" + part.materialName);
            node->mesh = part.mesh.get();
            node->material = part.material;
            SceneNode* n = g->addChild(std::move(node));
            if (part.materialName == "paint") paint = &n->material;   // the configurator changes this one
        }
    }
    if (paint) applyPaint(PAINTS[1]);
    else buildPlaceholderCar(car);                                     // model without a "paint" material
}

// Placeholder car built from primitives (used when the OBJ model is missing).
void Scene::buildPlaceholderCar(SceneNode* car) {
    Material body{PAINTS[1].color, PAINTS[1].shininess, PAINTS[1].specular};
    paint = &addBox(car, "body", {0.0f, 0.55f, 0.0f}, {3.6f, 0.55f, 1.6f}, body)->material;

    Material glass{{0.02f, 0.03f, 0.04f}, 128.0f, 1.0f};
    addBox(car, "cabin", {-0.25f, 1.08f, 0.0f}, {1.9f, 0.5f, 1.4f}, glass);

    Material headlight{{1, 1, 1}, 64.0f, 0.5f, {1.0f, 0.95f, 0.8f}};
    Material taillight{{0.3f, 0, 0}, 64.0f, 0.5f, {0.8f, 0.02f, 0.02f}};
    for (float z : {-0.55f, 0.55f}) {
        addBox(car, "headlight", {1.8f, 0.65f, z}, {0.04f, 0.14f, 0.35f}, headlight);
        addBox(car, "taillight", {-1.8f, 0.65f, z}, {0.04f, 0.12f, 0.35f}, taillight);
    }

    // Wheels: group node (position + spin) -> tyre, rim and spoke.
    // The group is unscaled, so scaling the tyre does not affect its children.
    Material tyre{{0.02f, 0.02f, 0.02f}, 8.0f, 0.1f};
    Material rim{{0.6f, 0.6f, 0.62f}, 96.0f, 0.9f};
    const glm::vec3 wheelPos[4] = {{1.15f, 0.35f, 0.8f}, {1.15f, 0.35f, -0.8f},
                                   {-1.15f, 0.35f, 0.8f}, {-1.15f, 0.35f, -0.8f}};
    for (int i = 0; i < 4; ++i) {
        auto wheel = std::make_unique<SceneNode>("wheel");
        wheel->position = wheelPos[i];
        wheel->rotation = {90.0f, 0.0f, 0.0f};          // cylinder axis Y -> Z (the axle)
        SceneNode* w = car->addChild(std::move(wheel));
        wheels_[i] = w;

        auto t = std::make_unique<SceneNode>("tyre");
        t->mesh = cylinder_.get();
        t->scale = {0.7f, 0.3f, 0.7f};
        t->material = tyre;
        w->addChild(std::move(t));

        auto r = std::make_unique<SceneNode>("rim");
        r->mesh = cylinder_.get();
        r->scale = {0.42f, 0.32f, 0.42f};
        r->material = rim;
        w->addChild(std::move(r));

        addBox(w, "spoke", {0, 0, 0}, {0.4f, 0.34f, 0.07f}, rim);
    }
}

void Scene::buildLights() {
    lights = {
        {"Key light (moving)", {4.0f, 5.0f, 3.0f}, {1.0f, 0.95f, 0.85f}, 1.2f},
        {"Fill light",         {-5.0f, 3.0f, 4.0f}, {0.6f, 0.7f, 1.0f}, 0.6f},
        {"Rim light",          {0.0f, 6.0f, -6.0f}, {1.0f, 1.0f, 1.0f}, 0.9f},
    };
    for (auto& l : lights) {
        auto m = std::make_unique<SceneNode>("light marker");
        m->mesh = sphere_.get();
        m->scale = glm::vec3(0.3f);
        l.marker = root_->addChild(std::move(m));
    }
}

void Scene::update(float dt) {
    SceneNode* tt = turntable_;
    if (!turntablePaused) tt->rotation.y += turntableSpeed * dt;

    for (SceneNode* w : wheels_)
        if (w) w->rotation.z -= wheelSpin * dt;          // axle is the Z axis

    if (keyLightOrbits) keyLightAngle_ += 25.0f * dt;
    float a = glm::radians(keyLightAngle_);
    lights[0].position = {5.0f * std::cos(a), 5.0f, 5.0f * std::sin(a)};

    // Markers glow in the light's colour when on, dark when off.
    for (auto& l : lights) {
        l.marker->position = l.position;
        l.marker->material = {{0.05f, 0.05f, 0.05f}, 8.0f, 0.0f,
                              l.enabled ? l.color : glm::vec3(0.02f)};
    }
}

void Scene::uploadLights(const Shader& shader) const {
    shader.setInt("uNumLights", static_cast<int>(lights.size()));
    shader.setFloat("uAmbient", ambient);
    for (size_t i = 0; i < lights.size() && i < MAX_LIGHTS; ++i) {
        std::string b = "uLights[" + std::to_string(i) + "].";
        shader.setVec3(b + "position", lights[i].position);
        shader.setVec3(b + "color", lights[i].color);
        shader.setFloat(b + "intensity", lights[i].enabled ? lights[i].intensity : 0.0f);
    }
}

void Scene::draw(const Shader& shader, bool transparentPass) const {
    root_->draw(shader, glm::mat4(1.0f), transparentPass);
}
