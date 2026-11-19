#pragma once

#include "Camera.h"
#include "Cubemap.h"
#include "Scene.h"
#include "Shader.h"
#include "Skybox.h"

#include <memory>

class App {
public:
    App(int width, int height);

    void frame();
    void resize(int w, int h);
    void keyDown(unsigned char key);
    void keyUp(unsigned char key);
    void specialDown(int key);
    void specialUp(int key);
    void mouseButton(int button, int state, int x, int y);
    void mouseMotion(int x, int y);
    void mouseWheel(int dir);

private:
    int width_, height_;
    std::unique_ptr<Shader> shader_;
    std::unique_ptr<Skybox> skybox_;
    Scene scene_;
    Camera camera_;

    int lastTimeMs_ = 0;
    float fpsTimer_ = 0.0f;
    int fpsFrames_ = 0;
    float fps_ = 0.0f;
    float frameMs_ = 0.0f;
    bool msaa_ = true;

    bool keys_[256] = {};
    bool arrows_[4] = {};
    bool dragging_ = false;
    int lastMouseX_ = 0, lastMouseY_ = 0;

    bool showUI_ = true;
    int paintIndex_ = 1;

    void update(float dt);
    void render();
    void drawUI();
    void updateFps(float dt);
    void toggleAllLights();
};
