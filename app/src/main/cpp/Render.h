#ifndef PHYSICSTEST_RENDER_H
#define PHYSICSTEST_RENDER_H

#include <android/native_window.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string>

#include <glm/glm.hpp>

#include "Physics.h"

using namespace std;
using namespace glm;

class Render {
public:
    static Render& getInstance() {
        static Render instance;

        return instance;
    }

    Render(Render const&) = delete;
    void operator=(Render const&)  = delete;
private:
    Render();

    int initialized;

    ANativeWindow* window;

    void initializeWindow();
    void finalizeWindow();

    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    EGLint width, height;

    void initializeEGL();
    void finalizeEGL();

    GLuint cubeTexture, wallTexture, cubeBuffer;

    GLint projectionID, viewID, sizeID, translationID, rotationID, texID;

    vec3 cameraPosition;
    float cameraAngleX, cameraAngleZ;

    void initializeGL();
    void finalizeGL();

    void updateProjectionMatrix();
    void updateViewMatrix();

    void lookAtPoint(const vec3 point);

    void drawCube(const Cube* cube, GLuint tex, GLenum cullMode);
public:
    void initialize();
    void finalize();

    void setOutputWindow(ANativeWindow* window);

    void draw();
};

#endif //PHYSICSTEST_RENDER_H
