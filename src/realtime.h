#pragma once

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <unordered_map>
#include <vector>
#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QTime>
#include <QTimer>

#include "utils/scenedata.h"
#include "utils/sceneparser.h"
#include "utils/shaderloader.h"
#include "camera/camera.h"

#include "hdr.h"
#include "cameratrace.h"
#include "camerapath.h"

struct PrimitiveMeshGL { GLuint vao=0, vbo=0; GLsizei count=0; };
struct PrimitiveMeshKey {
    PrimitiveType type; int p1; int p2;
    // define equivalence for PrimitiveMeshKeys
    bool operator==(const PrimitiveMeshKey& o) const { return type==o.type && p1==o.p1 && p2==o.p2; }
};
struct PrimitiveMeshKeyHash {
    size_t operator()(const PrimitiveMeshKey& k) const {
        return ((size_t)k.type*73856093) ^ (k.p1*19349663) ^ (k.p2*83492791);
    }
};

// ======================================================================
// GLShape
// ======================================================================

struct GLShape {
    GLuint vao = 0;
    GLuint vbo = 0;
    int count = 0;
    GLenum mode = GL_TRIANGLES;
    glm::mat4 model;

    void destroy() {
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
        vao = vbo = 0;
        count = 0;
    }
};

// ======================================================================
// Realtime class
// ======================================================================

class Realtime : public QOpenGLWidget
{
    Q_OBJECT

public:
    Realtime(QWidget *parent = nullptr);

    void finish();
    void sceneChanged();
    void settingsChanged();
    void saveViewportImage(std::string filePath);

protected:
    void initCameraPath();
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void timerEvent(QTimerEvent *event) override;

private:
    // ==== Rendering + scene ====
    GLuint m_shader = 0;
    std::vector<GLShape> m_objects;
    RenderData m_renderData;

    // ==== Camera ====
    Camera m_camera;
    bool m_mouseDown = false;
    glm::vec2 m_prev_mouse_pos;
    glm::vec3 m_camPos;
    glm::vec3 m_camLook;
    glm::vec3 m_camUp;
    glm::mat4 m_model = glm::mat4(1);
    glm::mat4 m_normalModel = glm::mat4(1);
    glm::mat4 m_view  = glm::mat4(1);
    glm::mat4 m_proj  = glm::mat4(1);

    // ==== Input ====
    std::unordered_map<int, bool> m_keyMap;

    // ==== Timing ====
    QElapsedTimer m_elapsedTimer;
    int m_timer = 0;
    float m_devicePixelRatio = 1.f;

    // ==== HDR ====
    HDR m_hdr;

    // ==== Camera Trace (ExtraCredit2) ====
    CameraTrace m_camTrace;

    CameraPath m_camPath;
    bool m_followingPath = false;

    // ==== Shadow Mapping (new) ====
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


    //
    // Meshes
    //
    GLuint m_mesh_texture;
    std::unordered_map<PrimitiveMeshKey, PrimitiveMeshGL, PrimitiveMeshKeyHash> m_meshes;
    std::vector<float> buildVertices(const ScenePrimitive& primitive) const;
    PrimitiveMeshGL& getMesh(const ScenePrimitive& primitive);


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

    // ==== Internal helpers ====
    void rebuildScene();
    void buildShape(GLShape &shape,
                    const std::vector<float> &data,
                    const glm::mat4 &model);
    void shader(const RenderShapeData& shape, const std::vector<SceneLightData>& lights);
    void uploadLights();
    void paintGeometry();

    GLuint m_height_map;
    QImage m_image;
    void loadHeightMap2D(const std::string &filename);
    std::vector<float> loadOBJ(const std::string& filename);
};

