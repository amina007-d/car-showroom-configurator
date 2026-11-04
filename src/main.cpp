// Car Showroom Configurator - entry point.
// Creates an OpenGL 3.3 core context with freeglut, loads GL functions with
// GLAD, sets up Dear ImGui and forwards all GLUT callbacks to App.

#include <glad/glad.h>
#include <GL/freeglut.h>

#include <imgui.h>
#include <backends/imgui_impl_glut.h>
#include <backends/imgui_impl_opengl3.h>

#include "App.h"

#include <cstdio>
#include <exception>
#include <memory>

static std::unique_ptr<App> app;

static void shutdown() {
    if (!app) return;
    app.reset();                       // free GL objects while the context still exists
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGLUT_Shutdown();
    ImGui::DestroyContext();
}

static void onDisplay() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::NewFrame();

    app->frame();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glutSwapBuffers();
}

static void onIdle() { glutPostRedisplay(); }
static void onReshape(int w, int h)                 { ImGui_ImplGLUT_ReshapeFunc(w, h); app->resize(w, h); }
static void onKey(unsigned char c, int x, int y)    { ImGui_ImplGLUT_KeyboardFunc(c, x, y); app->keyDown(c); }
static void onKeyUp(unsigned char c, int x, int y)  { ImGui_ImplGLUT_KeyboardUpFunc(c, x, y); app->keyUp(c); }
static void onSpecial(int k, int x, int y)          { ImGui_ImplGLUT_SpecialFunc(k, x, y); app->specialDown(k); }
static void onSpecialUp(int k, int x, int y)        { ImGui_ImplGLUT_SpecialUpFunc(k, x, y); app->specialUp(k); }
static void onMouse(int b, int s, int x, int y)     { ImGui_ImplGLUT_MouseFunc(b, s, x, y); app->mouseButton(b, s, x, y); }
static void onMotion(int x, int y)                  { ImGui_ImplGLUT_MotionFunc(x, y); app->mouseMotion(x, y); }
static void onWheel(int b, int d, int x, int y)     { ImGui_ImplGLUT_MouseWheelFunc(b, d, x, y); app->mouseWheel(d); }

int main(int argc, char** argv) {
    const int WIDTH = 1280, HEIGHT = 720;

    glutInit(&argc, argv);
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutSetOption(GLUT_MULTISAMPLE, 4);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Car Showroom Configurator");

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glutGetProcAddress))) {
        std::fprintf(stderr, "Failed to load OpenGL functions (GLAD)\n");
        return 1;
    }
    std::printf("OpenGL %s | %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGLUT_Init();
    ImGui_ImplOpenGL3_Init("#version 330");

    try {
        app = std::make_unique<App>(WIDTH, HEIGHT);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    }

    glutDisplayFunc(onDisplay);
    glutIdleFunc(onIdle);
    glutReshapeFunc(onReshape);
    glutIgnoreKeyRepeat(1);
    glutKeyboardFunc(onKey);
    glutKeyboardUpFunc(onKeyUp);
    glutSpecialFunc(onSpecial);
    glutSpecialUpFunc(onSpecialUp);
    glutMouseFunc(onMouse);
    glutMotionFunc(onMotion);
    glutPassiveMotionFunc(onMotion);
    glutMouseWheelFunc(onWheel);
    glutCloseFunc(shutdown);

    glutMainLoop();
    shutdown();
    return 0;
}
