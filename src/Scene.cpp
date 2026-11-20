#include "Scene.h"

#include <cmath>

const std::array<PaintPreset, 4> Scene::PAINTS = {{
    {"Matte black",   {0.015f, 0.015f, 0.017f},   6.0f, 0.08f, 0.02f},
    {"Glossy red",    {0.55f, 0.02f, 0.02f},    128.0f, 1.00f, 0.15f},
    {"Pearl white",   {0.80f, 0.80f, 0.78f},     64.0f, 0.70f, 0.12f},
    {"Midnight blue", {0.02f, 0.05f, 0.20f},     96.0f, 0.90f, 0.15f},
}};

void Scene::applyPaint(const PaintPreset& p) {
    if (!paint) return;
    paint->color = p.color;
    paint->shininess = p.shininess;
    paint->specular = p.specular;
    paint->reflectivity = p.reflectivity;
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
    plane_ = Mesh::plane(24.0f, 12.0f);
    ground_ = Mesh::plane(400.0f, 1.0f);
    quad_ = Mesh::plane(1.0f, 1.0f);
    cylinder_ = Mesh::cylinder(48);
    sphere_ = Mesh::sphere(16, 24);

    floorTex_ = Texture::fromFile("assets/textures/floor.png");
    if (!floorTex_) floorTex_ = Texture::checker(256, 2, {0.62f, 0.62f, 0.60f}, {0.54f, 0.54f, 0.52f});

    root_ = std::make_unique<SceneNode>("root");
    buildEnvironmentMaps();
    buildStudio();
    buildLights();
    buildSpotlights();
}

SceneNode* Scene::addQuad(SceneNode* parent, const std::string& name, glm::vec3 pos,
                          glm::vec3 rotation, glm::vec2 size, const Material& mat) {
    auto node = std::make_unique<SceneNode>(name);
    node->mesh = quad_.get();
    node->position = pos;
    node->rotation = rotation;
    node->scale = {size.x, 1.0f, size.y};
    node->material = mat;
    return parent->addChild(std::move(node));
}

void Scene::buildRoom() {
    const float half = 12.0f, height = 7.0f;

    Material wall{{0.020f, 0.020f, 0.023f}, 8.0f, 0.05f};
    Material backWall{{0.028f, 0.027f, 0.030f}, 8.0f, 0.05f};
    Material ceiling{{0.012f, 0.012f, 0.014f}, 4.0f, 0.0f};

    SceneNode* back = addQuad(root_.get(), "back wall", {0.0f, height / 2, -half}, {90.0f, 0.0f, 0.0f}, {2 * half, height}, backWall);
    closedOnly_.push_back(back);
    wallMaterials_.push_back(&back->material);
    wallMaterials_.push_back(&addQuad(root_.get(), "front wall", {0.0f, height / 2,  half}, {-90.0f, 0.0f, 0.0f}, {2 * half, height}, wall)->material);
    wallMaterials_.push_back(&addQuad(root_.get(), "left wall",  {-half, height / 2, 0.0f}, {0.0f, 0.0f, -90.0f}, {height, 2 * half}, wall)->material);
    wallMaterials_.push_back(&addQuad(root_.get(), "right wall", { half, height / 2, 0.0f}, {0.0f, 0.0f, 90.0f},  {height, 2 * half}, wall)->material);
    ceilingMaterial_ = &addQuad(root_.get(), "ceiling",    {0.0f, height, 0.0f},      {180.0f, 0.0f, 0.0f}, {2 * half, 2 * half}, ceiling)->material;

    buildWindowWall(half, height);
    buildColumns(half, height);

    Material panel{{0.05f, 0.05f, 0.05f}, 4.0f, 0.0f, {0.95f, 0.92f, 0.84f}};
    for (int row = -1; row <= 1; row += 2)
        for (int col = -1; col <= 1; ++col)
            addQuad(root_.get(), "ceiling panel", {col * 5.0f, height - 0.02f, row * 3.0f},
                    {180.0f, 0.0f, 0.0f}, {3.2f, 1.1f}, panel);
}

void Scene::buildWindowWall(float half, float height) {
    Material glass;
    glass.color = {0.55f, 0.65f, 0.70f};
    glass.shininess = 128.0f;
    glass.specular = 1.0f;
    glass.alpha = 0.12f;
    glass.blend = true;
    glass.doubleSided = true;
    glass.reflectivity = 0.15f;

    Material frame{{0.06f, 0.06f, 0.065f}, 64.0f, 0.5f};

    const int panes = 8;
    const float paneWidth = 2 * half / panes;
    const float z = -half + 0.02f;
    for (int i = 0; i < panes; ++i) {
        float x = -half + paneWidth * (i + 0.5f);
        openOnly_.push_back(addQuad(root_.get(), "window glass", {x, height / 2, z}, {90.0f, 0.0f, 0.0f},
                                    {paneWidth, height}, glass));
    }
    for (int i = 0; i <= panes; ++i) {
        float x = -half + paneWidth * i;
        openOnly_.push_back(addBox(root_.get(), "window mullion", {x, height / 2, z}, {0.12f, height, 0.16f}, frame));
    }
    for (float y : {0.06f, 2.6f, height - 0.06f})
        openOnly_.push_back(addBox(root_.get(), "window transom", {0.0f, y, z}, {2 * half, 0.1f, 0.16f}, frame));
}

void Scene::buildColumns(float half, float height) {
    Material column{{0.045f, 0.045f, 0.05f}, 32.0f, 0.3f};
    for (float x : {-6.0f, 6.0f})
        for (float z : {-half + 1.4f, half - 1.4f})
            addBox(root_.get(), "column", {x, height / 2, z}, {0.8f, height, 0.8f}, column);
}

void Scene::buildStudio() {
    auto floor = std::make_unique<SceneNode>("floor");
    floor->mesh = plane_.get();
    floor->material.texture = floorTex_.get();
    floor->material.shininess = 96.0f;
    floor->material.specular = 0.6f;
    floor->material.reflectivity = 0.08f;
    floorMaterial_ = &root_->addChild(std::move(floor))->material;

    auto ground = std::make_unique<SceneNode>("street");
    ground->mesh = ground_.get();
    ground->position = {0.0f, -0.02f, 0.0f};
    ground->material.color = {0.035f, 0.035f, 0.038f};
    ground->material.shininess = 8.0f;
    ground->material.specular = 0.05f;
    root_->addChild(std::move(ground));

    buildRoom();

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

    for (int i = 0; i < 12; ++i) {
        float a = i / 12.0f * 6.2831853f;
        auto lamp = std::make_unique<SceneNode>("floor lamp");
        lamp->mesh = cylinder_.get();
        lamp->position = {2.6f * std::cos(a), 0.11f, 2.6f * std::sin(a)};
        lamp->scale = {0.12f, 0.02f, 0.12f};
        lamp->material = {{0.1f, 0.1f, 0.1f}, 8.0f, 0.0f, {0.9f, 0.85f, 0.7f}};
        root_->addChild(std::move(lamp));
    }

    buildCars(turntable_);
}

void Scene::buildCars(SceneNode* turntable) {
    addCar(turntable, "Porsche 911 Turbo (1975)",       "assets/models/porsche930/car.obj", 1);
    addCar(turntable, "Toyota Supra (1993)",            "assets/models/supra93/car.obj",    2);
    addCar(turntable, "Nissan Skyline GT-R R34 (1999)", "assets/models/r34/car.obj",        3);
    addCar(turntable, "Lamborghini Aventador",          "assets/models/aventador/car.obj",  0);

    if (cars.empty()) {
        Car c;
        c.name = "Placeholder";
        auto node = std::make_unique<SceneNode>("car");
        node->position = {0.0f, 0.1f, 0.0f};
        c.root = turntable->addChild(std::move(node));
        buildPlaceholderCar(c.root);
        c.paint = paint;
        c.wheels = placeholderWheels_;
        cars.push_back(std::move(c));
    }
    selectCar(0);
}

bool Scene::addCar(SceneNode* turntable, const std::string& name, const std::string& objPath, int paintPreset) {
    Car c;
    c.name = name;
    c.model = Model::load(objPath);
    if (!c.model) return false;

    auto node = std::make_unique<SceneNode>(name);
    node->position = {0.0f, 0.1f, 0.0f};
    c.root = turntable->addChild(std::move(node));

    int wheelCount = 0;
    for (const ModelObject& obj : c.model->objects) {
        auto group = std::make_unique<SceneNode>(obj.name);
        group->position = obj.pivot;
        SceneNode* g = c.root->addChild(std::move(group));
        if (obj.name.rfind("wheel", 0) == 0 && wheelCount < 4) c.wheels[wheelCount++] = g;

        for (const ModelPart& part : obj.parts) {
            auto n = std::make_unique<SceneNode>(obj.name + "/" + part.materialName);
            n->mesh = part.mesh.get();
            n->material = part.material;
            SceneNode* added = g->addChild(std::move(n));
            if (part.materialName == "paint") c.paint = &added->material;
        }
    }
    if (!c.paint) {
        c.root->visible = false;
        return false;
    }
    paint = c.paint;
    applyPaint(PAINTS[paintPreset]);
    cars.push_back(std::move(c));
    return true;
}

void Scene::selectCar(int index) {
    if (cars.empty()) return;
    int count = static_cast<int>(cars.size());
    selectedCar = (index % count + count) % count;
    for (int i = 0; i < count; ++i) cars[i].root->visible = (i == selectedCar);
    paint = cars[selectedCar].paint;
}

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

    Material tyre{{0.02f, 0.02f, 0.02f}, 8.0f, 0.1f};
    Material rim{{0.6f, 0.6f, 0.62f}, 96.0f, 0.9f};
    const glm::vec3 wheelPos[4] = {{1.15f, 0.35f, 0.8f}, {1.15f, 0.35f, -0.8f},
                                   {-1.15f, 0.35f, 0.8f}, {-1.15f, 0.35f, -0.8f}};
    for (int i = 0; i < 4; ++i) {
        auto wheel = std::make_unique<SceneNode>("wheel");
        wheel->position = wheelPos[i];
        wheel->rotation = {90.0f, 0.0f, 0.0f};
        SceneNode* w = car->addChild(std::move(wheel));
        placeholderWheels_[i] = w;

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
        {"Key light (moving)", {4.0f, 5.0f, 3.0f}, {1.0f, 1.0f, 1.0f}, 1.0f},
        {"Fill light",         {-5.0f, 3.0f, 4.0f}, {1.0f, 1.0f, 1.0f}, 1.0f},
        {"Rim light",          {0.0f, 6.0f, -6.0f}, {1.0f, 1.0f, 1.0f}, 1.0f},
    };
    for (auto& l : lights) {
        auto m = std::make_unique<SceneNode>("light marker");
        m->mesh = sphere_.get();
        m->scale = glm::vec3(0.3f);
        l.marker = root_->addChild(std::move(m));
    }
    applyEnvironment(0);
}

void Scene::buildSpotlights() {
    const glm::vec3 target{0.0f, 0.8f, 0.0f};
    const float height = 6.8f, radius = 2.6f;
    Material housing{{0.03f, 0.03f, 0.035f}, 32.0f, 0.4f};
    Material lens{{0.1f, 0.1f, 0.1f}, 8.0f, 0.0f, {1.0f, 0.96f, 0.88f}};

    for (int i = 0; i < 3; ++i) {
        float a = glm::radians(90.0f + 120.0f * i);
        glm::vec3 pos{radius * std::cos(a), height, radius * std::sin(a)};
        spots.push_back({"Spotlight " + std::to_string(i + 1), pos, glm::normalize(target - pos),
                         {1.0f, 0.96f, 0.88f}, 2.5f, 14.0f, 22.0f});

        auto body = std::make_unique<SceneNode>("spotlight housing");
        body->mesh = cylinder_.get();
        body->position = pos + glm::vec3(0.0f, 0.1f, 0.0f);
        body->scale = {0.4f, 0.25f, 0.4f};
        body->material = housing;
        root_->addChild(std::move(body));

        auto glow = std::make_unique<SceneNode>("spotlight lens");
        glow->mesh = cylinder_.get();
        glow->position = pos - glm::vec3(0.0f, 0.03f, 0.0f);
        glow->scale = {0.3f, 0.02f, 0.3f};
        glow->material = lens;
        root_->addChild(std::move(glow));
    }
}

void Scene::buildEnvironmentMaps() {
    for (const Environment& env : ENVIRONMENTS) {
        std::unique_ptr<Cubemap> map;
        if (!env.skyboxFolder.empty()) map = Cubemap::fromFolder(env.skyboxFolder);
        if (!map) map = Cubemap::gradient(64, env.skyTop, env.skyHorizon, env.skyBottom);
        environmentMaps_.push_back(std::move(map));
    }
}

void Scene::applyEnvironment(int index) {
    int count = static_cast<int>(ENVIRONMENTS.size());
    environment = (index % count + count) % count;
    const Environment& env = ENVIRONMENTS[environment];
    ambient = env.ambient;
    for (Material* m : wallMaterials_) m->color = env.wallColor;
    if (ceilingMaterial_) ceilingMaterial_->color = env.ceilingColor;
    if (floorMaterial_) {
        floorMaterial_->color = glm::vec3(env.floorTint);
        floorMaterial_->reflectivity = env.floorReflectivity;
    }
    for (SceneNode* n : closedOnly_) n->visible = env.walls;
    for (SceneNode* n : openOnly_) n->visible = !env.walls;
    for (size_t i = 0; i < lights.size() && i < env.lights.size(); ++i) {
        lights[i].color = env.lights[i].color;
        lights[i].intensity = env.lights[i].intensity;
    }
}

void Scene::update(float dt) {
    SceneNode* tt = turntable_;
    if (!turntablePaused) tt->rotation.y += turntableSpeed * dt;

    for (SceneNode* w : cars[selectedCar].wheels)
        if (w) w->rotation.z -= wheelSpin * dt;

    if (keyLightOrbits) keyLightAngle_ += 25.0f * dt;
    float a = glm::radians(keyLightAngle_);
    lights[0].position = {5.0f * std::cos(a), 5.0f, 5.0f * std::sin(a)};

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
    shader.setInt("uNumSpots", static_cast<int>(spots.size()));
    for (size_t i = 0; i < spots.size() && i < MAX_SPOTS; ++i) {
        std::string b = "uSpots[" + std::to_string(i) + "].";
        shader.setVec3(b + "position", spots[i].position);
        shader.setVec3(b + "direction", spots[i].direction);
        shader.setVec3(b + "color", spots[i].color);
        shader.setFloat(b + "intensity", spotlightsOn ? spots[i].intensity : 0.0f);
        shader.setFloat(b + "cosInner", std::cos(glm::radians(spots[i].innerDeg)));
        shader.setFloat(b + "cosOuter", std::cos(glm::radians(spots[i].outerDeg)));
    }
}

void Scene::draw(const Shader& shader, bool transparentPass) const {
    root_->draw(shader, glm::mat4(1.0f), transparentPass);
}
