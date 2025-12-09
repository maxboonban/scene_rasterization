#pragma once

// Defined before including GLEW to suppress deprecation messages on macOS
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "utils/sceneparser.h"
#include "camera/camera.h"

#include <unordered_map>
#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QTime>
#include <QTimer>
#include "iostream"

// For building and keeping track of primitive meshes
struct PrimitiveMeshGL { GLuint vao=0, vbo=0; GLsizei count=0; };
// struct PrimitiveMeshKey {
//     PrimitiveType type; int p1; int p2;
//     // define equivalence for PrimitiveMeshKeys
//     bool operator==(const PrimitiveMeshKey& o) const { return type==o.type && p1==o.p1 && p2==o.p2; }
// };
struct PrimitiveMeshKey {
    PrimitiveType type; int p1; int p2; std::string matInfo;
    // define equivalence for PrimitiveMeshKeys
    bool operator==(const PrimitiveMeshKey& o) const { return type==o.type && p1==o.p1 && p2==o.p2 && matInfo==o.matInfo; }
};
struct PrimitiveMeshKeyHash {
    size_t operator()(const PrimitiveMeshKey& k) const {
        return ((size_t)k.type*73856093) ^ (k.p1*19349663) ^ (k.p2*83492791);
    }
};

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

    // Parsed scene data
    RenderData m_metaData;

    // Camera/object space matrices
    glm::mat4 m_model = glm::mat4(1);
    glm::mat4 m_normalModel = glm::mat4(1);
    glm::mat4 m_view  = glm::mat4(1);
    glm::mat4 m_proj  = glm::mat4(1);

    // Shader and vbo/vaos
    std::unordered_map<PrimitiveMeshKey, PrimitiveMeshGL, PrimitiveMeshKeyHash> m_meshes; // Stores meshes for each unique primitive/p1/p2 combo
    GLuint m_shader;                                                                      // Stores id of shader program

    //
    // Trimeshes
    //
    // Helper to build vertices for a given primitive
    std::vector<float> buildVertices(const ScenePrimitive& primitive) const;
    // Helper to get mesh for the current primitive and tessellation parameters
    // Builds vertices if does not already exist
    PrimitiveMeshGL& getMesh(const ScenePrimitive& primitive);
    // load obj file
    std::vector<float> loadOBJ(const std::string& filename);

    //
    // Phong
    //
    // Helper to pass light and camera info to the shader for phong illumination
    void shader(const RenderShapeData& shape, const std::vector<SceneLightData>& lights);
    void paintGeometry();

    //
    // Camera Movement
    //
    glm::vec3 m_camPos;
    glm::vec3 m_camLook;
    glm::vec3 m_camUp;
    float m_angleX;
    float m_angleY;
    // Computes the rotation matrix about the given axis for the given angle in radians
    glm::mat4 rodrigues(const float theta, const glm::vec3 &axis);
    // Rotates the camera look and up
    void updateMouseRotation();

    //
    // FBOs
    //
    GLuint m_fullscreen_vbo;
    GLuint m_fullscreen_vao;
    GLuint m_defaultFBO;
    GLuint m_fbo;
    GLuint m_fbo_texture;
    GLuint m_fbo_depth;
    GLuint m_texture_shader;
    int m_screen_width;
    int m_screen_height;
    int m_fbo_width;
    int m_fbo_height;
    void makeFBO();
    void initializeFBO();
    void paintTexture(GLuint colorTexture, GLuint depthTexture);

    // Bloom
    GLuint m_fbo_bloom_bright;      // Texture for bright colors extracted from scene
    GLuint m_fbo_pingpong[2];       // Ping-pong FBOs for blurring
    GLuint m_fbo_pingpong_texture[2]; // Textures for ping-pong blur
    GLuint m_bloom_extract_shader;   // Shader to extract bright colors
    GLuint m_blur_shader;            // Shader for Gaussian blur
    GLuint m_bloom_composite_shader; // Shader to composite scene + bloom
    void initializeBloomFBOs();
    void renderBloom();

    //
    // Shadow mapping
    //
    GLuint m_fbo_shadow;
    std::vector<GLuint> m_shadow_maps;    // for directional and/or spot lights
    std::vector<glm::mat4> m_light_MVPs;  // for directional and/or spot lights
    int m_shadow_size = 1024;
    GLuint m_shadow_shader;
    int numLights;  // number of total lights (max 8)
    void initializeShadowDepths();
    void initializeShadowFBO();
    void computeLightMVPs();
    void paintShadows();

    //
    // bump mapping
    //
    GLuint m_height_map;
    QImage m_image;
    void loadHeightMap2D(const std::string &filename);
};
