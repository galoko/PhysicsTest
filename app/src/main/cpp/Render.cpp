#include "Render.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "log.h"
#include "exceptionUtils.h"

#include "AssetManager.h"

#define RENDER_TAG "PT_RENDER"

#undef DEPTH_TEST

Render::Render() {

}

void Render::initialize() {

    if (this->initialized == 1)
        return;

    this->initialized = 1;
}

void Render::finalize() {

    if (this->initialized == 0)
        return;

    setOutputWindow(nullptr);

    this->initialized = 0;
}

void Render::setOutputWindow(ANativeWindow* window) {

    if (this->window) {
        finalizeWindow();

        ANativeWindow_release(this->window);
        this->window = nullptr;
    }

    this->window = window;

    if (this->window) {
        initializeWindow();
    }
}

void Render::initializeWindow() {

    initializeEGL();
    initializeGL();
}

void Render::finalizeWindow() {

    finalizeGL();
    finalizeEGL();
}

void Render::initializeEGL() {
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglCheckError(display != EGL_NO_DISPLAY, "eglGetDisplay");

    eglCheckError(eglInitialize(display, nullptr, nullptr) == EGL_TRUE, "eglInitialize");

    const EGLint configAttributes[] = {
            EGL_SURFACE_TYPE,    /* = */ EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, /* = */ 0,
#ifdef DEPTH_TEST
            EGL_DEPTH_SIZE,      /* = */ 24,
#endif
            EGL_BLUE_SIZE,       /* = */ 8,
            EGL_GREEN_SIZE,      /* = */ 8,
            EGL_RED_SIZE,        /* = */ 8,
            EGL_CONFORMANT,      /* = */ EGL_OPENGL_ES2_BIT,
            EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;
    eglCheckError(eglChooseConfig(display, configAttributes, &config, 1, &numConfigs) == EGL_TRUE, "eglChooseConfig");

    eglCheckError(numConfigs == 1, "eglChooseConfig.numConfigs");

    EGLint format;
    eglCheckError(eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format) == EGL_TRUE, "eglGetConfigAttrib");

    my_assert(ANativeWindow_setBuffersGeometry(window, 0, 0, format) == 0);

    EGLSurface surface = eglCreateWindowSurface(display, config, window, nullptr);
    eglCheckError(surface != EGL_NO_SURFACE, "eglCreateWindowSurface");

    const EGLint contextAttributes[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
    };
    EGLContext context = eglCreateContext(display, config, nullptr, contextAttributes);
    eglCheckError(context != EGL_NO_CONTEXT, "eglCreateContext");

    eglCheckError(eglMakeCurrent(display, surface, surface, context) == EGL_TRUE, "eglMakeCurrent");

    EGLint width;
    eglCheckError(eglQuerySurface(display, surface, EGL_WIDTH, &width) == EGL_TRUE, "eglQuerySurface.EGL_WIDTH");

    EGLint height;
    eglCheckError(eglQuerySurface(display, surface, EGL_HEIGHT, &height) == EGL_TRUE, "eglQuerySurface.EGL_HEIGHT");

    this->display = display;
    this->surface = surface;
    this->context = context;

    this->width = width;
    this->height = height;
}

void Render::finalizeEGL() {

    // TODO
    throw runtime_error("not done");
}

void Render::initializeGL() {

    glViewport(0, 0, width, height);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

#ifdef DEPTH_TEST
    glEnable(GL_DEPTH_TEST);
#endif

    // shaders

    string vertexShaderCode = AssetManager::getInstance().loadTextAsset("scene.vertexshader");
    string fragmentShaderCode = AssetManager::getInstance().loadTextAsset("scene.fragmentshader");

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    char const * codePointer;

    codePointer = vertexShaderCode.c_str();
    glShaderSource(vertexShader, 1, &codePointer, nullptr);

    codePointer = fragmentShaderCode.c_str();
    glShaderSource(fragmentShader, 1, &codePointer, nullptr);

    GLint res = GL_FALSE;

    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &res);
    my_assert(res == GL_TRUE);

    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &res);
    my_assert(res == GL_TRUE);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &res);
    my_assert(res == GL_TRUE);

    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glUseProgram(program);

    // color

    glClearColor(1, 0, 0, 1);

    // uniforms

    projectionID = glGetUniformLocation(program, "projection");
    viewID = glGetUniformLocation(program, "view");

    sizeID = glGetUniformLocation(program, "size");
    translationID = glGetUniformLocation(program, "translation");
    rotationID = glGetUniformLocation(program, "rotation");

    texID = glGetUniformLocation(program, "tex");

    // setup texture

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(texID, 0);

    // textures

    cubeTexture = AssetManager::getInstance().loadTextureAsset("cube.bmp");
    wallTexture = AssetManager::getInstance().loadTextureAsset("wall.bmp");

    // vertices

    float vertices[6 * 3 * 2 * (3 + 3 + 2)] =
    {
        -0.50f,  0.50f,  0.50f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         0.50f,  0.50f, -0.50f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
        -0.50f,  0.50f, -0.50f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
         0.50f,  0.50f,  0.50f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.50f, -0.50f, -0.50f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.50f,  0.50f, -0.50f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.50f, -0.50f,  0.50f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.50f, -0.50f, -0.50f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
         0.50f, -0.50f, -0.50f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.50f, -0.50f,  0.50f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.50f,  0.50f, -0.50f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.50f, -0.50f, -0.50f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.50f, -0.50f, -0.50f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
        -0.50f,  0.50f, -0.50f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
         0.50f,  0.50f, -0.50f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.50f,  0.50f,  0.50f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
        -0.50f, -0.50f,  0.50f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
         0.50f, -0.50f,  0.50f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        -0.50f,  0.50f,  0.50f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         0.50f,  0.50f,  0.50f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         0.50f,  0.50f, -0.50f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         0.50f,  0.50f,  0.50f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.50f, -0.50f,  0.50f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         0.50f, -0.50f, -0.50f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.50f, -0.50f,  0.50f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.50f, -0.50f,  0.50f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
        -0.50f, -0.50f, -0.50f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
        -0.50f, -0.50f,  0.50f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.50f,  0.50f,  0.50f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.50f,  0.50f, -0.50f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.50f, -0.50f, -0.50f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
        -0.50f, -0.50f, -0.50f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.50f,  0.50f, -0.50f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
         0.50f,  0.50f,  0.50f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
        -0.50f,  0.50f,  0.50f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.50f, -0.50f,  0.50f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f
    };

    glGenBuffers(1, &cubeBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, cubeBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    const unsigned int SIZE_OF_VERTEX = (3 + 3 + 2) * sizeof(float);

    GLint index;

    index = glGetAttribLocation(program, "vertexPosition");
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 3, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void *)(0 * sizeof(float)));

    index = glGetAttribLocation(program, "vertexNormal");
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 3, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void *)(3 * sizeof(float)));

    index = glGetAttribLocation(program, "vertexTexCoord");
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void *)((3 + 3) * sizeof(float)));

    // setup matrices

    updateProjectionMatrix();

    cameraPosition = { 2, 2, 2 };
    lookAtPoint({ 0, 0, 0 });
    updateViewMatrix();
}

void Render::finalizeGL() {

    // TODO
    throw runtime_error("not done");
}

void Render::updateProjectionMatrix() {
    float aspectRatio = (float)width / (float)height;
    mat4 projection = perspective(radians(45.0f), aspectRatio, 0.1f, 100.0f);
    glUniformMatrix4fv(projectionID, 1, GL_FALSE, value_ptr(projection));
}

void Render::updateViewMatrix() {

    mat4 view;

    // limit angles
    cameraAngleZ /= (float)M_PI * 2.0f;
    cameraAngleZ -= (float)(long)cameraAngleZ;
    cameraAngleZ *= (float)M_PI * 2.0f;

    cameraAngleX = std::min(std::max(0.1f, cameraAngleX), 3.13f);

    vec3 direction;

    float cameraAngleXsin, cameraAngleXcos, cameraAngleZsin, cameraAngleZcos;
    sincosf(cameraAngleX, &cameraAngleXsin, &cameraAngleXcos);
    sincosf(cameraAngleZ, &cameraAngleZsin, &cameraAngleZcos);

    direction.x = cameraAngleXsin * cameraAngleZsin;
    direction.y = cameraAngleXsin * cameraAngleZcos;
    direction.z = cameraAngleXcos;

    const vec3 up = { 0, 0, 1 };

    view = lookAt(cameraPosition, cameraPosition + direction, up);

    glUniformMatrix4fv(viewID, 1, GL_FALSE, value_ptr(view));
}

void Render::lookAtPoint(const vec3 point) {

    vec3 direction = normalize(point - cameraPosition);

    float sinX = sqrt(1 - direction.z * direction.z);

    cameraAngleZ = atan2(direction.x / sinX, direction.y / sinX);
    cameraAngleX = atan2(sinX, direction.z);

    updateViewMatrix();
}

void Render::drawCube(const Cube* cube, GLuint tex, GLenum cullMode) {

    glBindTexture(GL_TEXTURE_2D, tex);
    glCullFace(cullMode);

    glUniform3fv(translationID, 1, value_ptr(cube->getPosition()));
    glUniformMatrix3fv(rotationID, 1, GL_FALSE, value_ptr(cube->getRotation()));
    glUniform3fv(sizeID, 1, value_ptr(cube->getSize()));

    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Render::draw() {

    // cameraPosition.z = Physics::getInstance().getCube()->getPosition().z;
    lookAtPoint(Physics::getInstance().getCube()->getPosition());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawCube(Physics::getInstance().getWalls(), wallTexture, GL_FRONT);

    drawCube(Physics::getInstance().getCube(), cubeTexture, GL_BACK);

    // glFlush(); // do we need this or what?

    eglSwapBuffers(display, surface);
}