#pragma once

// Defined before including GLEW to suppress deprecation messages on macOS
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <utils/shaderloader.h>
#include <utils/sceneparser.h>
#include <utils/scenedata.h>
#include <shapes/Cone.h>
#include <shapes/Cube.h>
#include <shapes/Cylinder.h>
#include <shapes/Sphere.h>
#include <camera.h>
#include <transforms.h>

#include <unordered_map>
#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QTime>
#include <QTimer>

class Realtime : public QOpenGLWidget
{
public:
    Realtime(QWidget *parent = nullptr);
    void finish();                                      // Called on program exit
    void sceneChanged();
    void settingsChanged();
    void saveViewportImage(std::string filePath);

public slots:
    void tick(QTimerEvent* event);                      // Called once per tick of m_timer

protected:
    void initializeGL() override;                       // Called once at the start of the program
    void paintGL() override;                            // Called whenever the OpenGL context changes or by an update() request
    void resizeGL(int width, int height) override;      // Called when window size changes

private:
    GLuint m_shader; // Stores id of shader program
    GLuint m_vao;    // Stores id of VAO
    GLuint m_vbo;

    GLuint m_fullscreen_vao;
    GLuint m_durand;
    GLuint m_defaultFBO = 2;
    GLuint m_fbo;
    GLuint m_fbo_texture;
    GLuint m_fbo_renderbuffer;

    glm::mat4 M0;
    glm::mat4 M1;
    glm::mat4 M2;
    glm::mat4 M3;
    float t;

    std::vector<GLfloat> coneData;
    std::vector<GLfloat> cubeData;
    std::vector<GLfloat> cylinderData;
    std::vector<GLfloat> sphereData;

    std::vector<GLfloat> fullscreen_quad_data;

    RenderData sceneData;
    Camera camera;

    std::vector<GLfloat> selectPrimitiveTris(const RenderShapeData &shape);
    void passUniformArgs(GLuint shader, const RenderShapeData &shape);
    void regenerateTris(int param_1, int param_2);
    void makeFBO();
    void paintTexture(GLuint texture);

    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

    // Tick Related Variables
    int m_timer;                                        // Stores timer which attempts to run ~60 times per second
    QElapsedTimer m_elapsedTimer;                       // Stores timer which keeps track of actual time between frames

    // Input Related Variables
    bool m_mouseDown = false;                           // Stores state of left mouse button
    glm::vec2 m_prev_mouse_pos;                         // Stores mouse position
    std::unordered_map<Qt::Key, bool> m_keyMap;         // Stores whether keys are pressed or not

    // Device Correction Variables
    double m_devicePixelRatio;
};
