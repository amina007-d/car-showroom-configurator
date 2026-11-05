#include "App.h"

#include <GL/freeglut.h>
#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cstdio>

App::App(int width, int height) : width_(width), height_(height) {
    shader_ = std::make_unique<Shader>("shaders/phong.vert", "shaders/phong.frag");
    scene_.build();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);       // all meshes use counter-clockwise front faces
    glEnable(GL_MULTISAMPLE);
    lastTimeMs_ = glutGet(GLUT_ELAPSED_TIME);
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------

void App::frame() {
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = std::min((now - lastTimeMs_) / 1000.0f, 0.1f);   // seconds, clamped after stalls
    lastTimeMs_ = now;

    update(dt);
    updateFps(dt);
    render();
}

void App::update(float dt) {
    // Keyboard camera: speeds are per second, multiplied by dt -> frame-rate independent.
    const float rotSpeed = 90.0f, zoomSpeed = 6.0f;
    float yaw = 0, pitch = 0, zoom = 0;
    if (keys_['a'] || arrows_[0]) yaw -= 1;
    if (keys_['d'] || arrows_[1]) yaw += 1;
    if (keys_['w'] || arrows_[2]) pitch += 1;
    if (keys_['s'] || arrows_[3]) pitch -= 1;
    if (keys_['q']) zoom -= 1;
    if (keys_['e']) zoom += 1;
    camera_.rotate(yaw * rotSpeed * dt, pitch * rotSpeed * dt);
    camera_.zoom(zoom * zoomSpeed * dt);

    scene_.update(dt);
}

void App::updateFps(float dt) {
    fpsTimer_ += dt;
    ++fpsFrames_;
    if (fpsTimer_ >= 0.5f) {
        fps_ = fpsFrames_ / fpsTimer_;
        fpsTimer_ = 0.0f;
        fpsFrames_ = 0;
        char title[96];
        std::snprintf(title, sizeof(title), "Car Showroom Configurator | FPS: %.1f", fps_);
        glutSetWindowTitle(title);                              // R7: FPS counter
    }
}

void App::render() {
    glViewport(0, 0, width_, height_);
    glClearColor(0.02f, 0.02f, 0.025f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader_->use();
    shader_->setMat4("uView", camera_.view());
    shader_->setMat4("uProjection", camera_.projection(static_cast<float>(width_) / std::max(height_, 1)));
    shader_->setVec3("uViewPos", camera_.position());
    shader_->setInt("uTexture", 0);
    scene_.uploadLights(*shader_);

    // Pass 1: opaque geometry. Pass 2: transparent geometry (glass, lights, shadow)
    // blended over it, without writing depth so it does not hide what is behind it.
    scene_.draw(*shader_, false);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    scene_.draw(*shader_, true);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    drawUI();
}

// ---------------------------------------------------------------------------
// ImGui panel (C3: on-screen controls help)
// ---------------------------------------------------------------------------

void App::drawUI() {
    if (!showUI_) return;

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(330, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Configurator");
    ImGui::Text("FPS: %.1f", fps_);

    if (ImGui::CollapsingHeader("Paint", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (int i = 0; i < static_cast<int>(Scene::PAINTS.size()); ++i) {
            if (i > 0) ImGui::SameLine();
            if (ImGui::Button(Scene::PAINTS[i].name)) {
                paintIndex_ = i;
                scene_.applyPaint(Scene::PAINTS[i]);
            }
            if (i == 1) ImGui::NewLine();
        }
        Material& p = *scene_.paint;
        ImGui::ColorEdit3("Colour", &p.color.x);
        ImGui::SliderFloat("Shininess", &p.shininess, 1.0f, 256.0f, "%.0f", ImGuiSliderFlags_Logarithmic);
        ImGui::SliderFloat("Specular", &p.specular, 0.0f, 1.0f);
        if (ImGui::Button("Matte")) { p.shininess = 6.0f; p.specular = 0.08f; }
        ImGui::SameLine();
        if (ImGui::Button("Glossy")) { p.shininess = 128.0f; p.specular = 1.0f; }
    }

    if (ImGui::CollapsingHeader("Turntable", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Paused", &scene_.turntablePaused);
        ImGui::SliderFloat("Speed (deg/s)", &scene_.turntableSpeed, -90.0f, 90.0f);
        ImGui::SliderFloat("Wheel spin", &scene_.wheelSpin, 0.0f, 720.0f);
    }

    if (ImGui::CollapsingHeader("Lights")) {
        ImGui::SliderFloat("Ambient", &scene_.ambient, 0.0f, 0.5f);
        ImGui::Checkbox("Key light orbits", &scene_.keyLightOrbits);
        for (size_t i = 0; i < scene_.lights.size(); ++i) {
            PointLight& l = scene_.lights[i];
            ImGui::PushID(static_cast<int>(i));
            ImGui::Checkbox(l.name.c_str(), &l.enabled);
            ImGui::SliderFloat("Intensity", &l.intensity, 0.0f, 3.0f);
            ImGui::ColorEdit3("Light colour", &l.color.x, ImGuiColorEditFlags_NoInputs);
            ImGui::PopID();
        }
    }

    if (ImGui::CollapsingHeader("Controls")) {
        ImGui::TextUnformatted(
            "Mouse drag     orbit camera\n"
            "Mouse wheel    zoom\n"
            "WASD / arrows  orbit camera\n"
            "Q / E          zoom in / out\n"
            "R              reset camera\n"
            "P              next paint preset\n"
            "M              toggle matte / glossy\n"
            "Space          pause turntable\n"
            "[ / ]          turntable speed\n"
            "1 2 3          toggle lights\n"
            "L              key light orbit on/off\n"
            "V              spin wheels\n"
            "H              hide / show this panel\n"
            "Esc            quit");
    }
    if (ImGui::Button("Reset camera")) camera_.reset();
    ImGui::End();
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void App::resize(int w, int h) {
    width_ = w;
    height_ = h;
}

void App::keyDown(unsigned char key) {
    if (ImGui::GetIO().WantCaptureKeyboard) return;
    key = static_cast<unsigned char>(std::tolower(key));
    keys_[key] = true;

    Material& p = *scene_.paint;
    switch (key) {
        case 27:  glutLeaveMainLoop(); break;
        case ' ': scene_.turntablePaused = !scene_.turntablePaused; break;
        case '[': scene_.turntableSpeed -= 10.0f; break;
        case ']': scene_.turntableSpeed += 10.0f; break;
        case 'r': camera_.reset(); break;
        case 'h': showUI_ = !showUI_; break;
        case 'l': scene_.keyLightOrbits = !scene_.keyLightOrbits; break;
        case 'v': scene_.wheelSpin = scene_.wheelSpin > 0 ? 0.0f : 360.0f; break;
        case 'p':
            paintIndex_ = (paintIndex_ + 1) % static_cast<int>(Scene::PAINTS.size());
            scene_.applyPaint(Scene::PAINTS[paintIndex_]);
            break;
        case 'm':
            if (p.shininess > 32.0f) { p.shininess = 6.0f;   p.specular = 0.08f; }
            else                     { p.shininess = 128.0f; p.specular = 1.0f; }
            break;
        case '1': case '2': case '3': {
            size_t i = key - '1';
            if (i < scene_.lights.size()) scene_.lights[i].enabled = !scene_.lights[i].enabled;
            break;
        }
        default: break;
    }
}

void App::keyUp(unsigned char key) {
    keys_[static_cast<unsigned char>(std::tolower(key))] = false;
}

static int arrowIndex(int key) {
    switch (key) {
        case GLUT_KEY_LEFT: return 0;
        case GLUT_KEY_RIGHT: return 1;
        case GLUT_KEY_UP: return 2;
        case GLUT_KEY_DOWN: return 3;
        default: return -1;
    }
}

void App::specialDown(int key) {
    if (ImGui::GetIO().WantCaptureKeyboard) return;
    int i = arrowIndex(key);
    if (i >= 0) arrows_[i] = true;
}

void App::specialUp(int key) {
    int i = arrowIndex(key);
    if (i >= 0) arrows_[i] = false;
}

void App::mouseButton(int button, int state, int x, int y) {
    if (button == 3 || button == 4) {                // some systems report the wheel as buttons
        if (state == GLUT_DOWN) mouseWheel(button == 3 ? 1 : -1);
        return;
    }
    if (button == GLUT_LEFT_BUTTON) {
        dragging_ = (state == GLUT_DOWN) && !ImGui::GetIO().WantCaptureMouse;
        lastMouseX_ = x;
        lastMouseY_ = y;
    }
}

void App::mouseMotion(int x, int y) {
    if (!dragging_) return;
    const float sensitivity = 0.3f;                  // degrees per pixel
    camera_.rotate(-(x - lastMouseX_) * sensitivity, (y - lastMouseY_) * sensitivity);
    lastMouseX_ = x;
    lastMouseY_ = y;
}

void App::mouseWheel(int dir) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    camera_.zoom(-dir * 0.5f);
}
