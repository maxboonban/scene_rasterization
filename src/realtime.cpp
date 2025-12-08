#include "realtime.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "settings.h"
#include "utils/sceneparser.h"
#include "shapes/Cube.h"
#include "shapes/Cone.h"
#include "shapes/Sphere.h"
#include "shapes/Cylinder.h"

// ======================================================================
// Constructor
// ======================================================================

Realtime::Realtime(QWidget *parent)
    : QOpenGLWidget(parent)
{
    m_prev_mouse_pos = glm::vec2(size().width()/2, size().height()/2);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_keyMap[Qt::Key_W]       = false;
    m_keyMap[Qt::Key_A]       = false;
    m_keyMap[Qt::Key_S]       = false;
    m_keyMap[Qt::Key_D]       = false;
    m_keyMap[Qt::Key_Control] = false;
    m_keyMap[Qt::Key_Space]   = false;
}

// ======================================================================
// Cleanup
// ======================================================================

void Realtime::finish() {
    killTimer(m_timer);
    makeCurrent();

    // --- Analytic primitive VAOs/VBOs ---
    for (auto &obj : m_objects) {
        obj.destroy();
    }
    m_objects.clear();

    // --- Trimesh VAOs/VBOs (if used) ---
    for (const auto &pair : m_meshes) {
        if (pair.second.vbo) glDeleteBuffers(1, &pair.second.vbo);
        if (pair.second.vao) glDeleteVertexArrays(1, &pair.second.vao);
    }
    m_meshes.clear();

    // --- Shader programs ---
    if (m_shader)         glDeleteProgram(m_shader);
    if (m_texture_shader) glDeleteProgram(m_texture_shader);
    if (m_shadow_shader)  glDeleteProgram(m_shadow_shader);

    // --- Fullscreen quad / FBO (your FBO for paintTexture) ---
    if (m_fullscreen_vao) glDeleteVertexArrays(1, &m_fullscreen_vao);
    if (m_fullscreen_vbo) glDeleteBuffers(1, &m_fullscreen_vbo);

    if (m_fbo_texture) glDeleteTextures(1, &m_fbo_texture);
    if (m_fbo_depth)   glDeleteTextures(1, &m_fbo_depth);
    if (m_fbo)         glDeleteFramebuffers(1, &m_fbo);

    // --- Shadow resources ---
    if (!m_shadow_maps.empty()) {
        glDeleteTextures(static_cast<GLsizei>(m_shadow_maps.size()),
                         m_shadow_maps.data());
        m_shadow_maps.clear();
    }
    if (m_fbo_shadow) glDeleteFramebuffers(1, &m_fbo_shadow);

    // --- HDR + camera trace systems ---
    m_hdr.destroy();
    m_camTrace.destroy();

    doneCurrent();
}

// ======================================================================
// Hard-coded camera path
// ======================================================================

void Realtime::initCameraPath() {
    // Canonical path in some local space
    m_camPath.pts = {
        glm::vec3(8, 8, 8),
        glm::vec3(8, -8, 8),
        glm::vec3(-8, -8, 8),
        glm::vec3(8, 8, 8)
    };

    m_camPath.durationSec = 30.f;
    m_camPath.reset();
    m_followingPath = false;
}

// ======================================================================
// VAO/VBO builder for analytic primitives
// ======================================================================

void Realtime::buildShape(GLShape &shape,
                          const std::vector<float> &data,
                          const glm::mat4 &M)
{
    shape.destroy();
    shape.count = static_cast<int>(data.size() / 6);

    glGenVertexArrays(1, &shape.vao);
    glGenBuffers(1, &shape.vbo);

    glBindVertexArray(shape.vao);
    glBindBuffer(GL_ARRAY_BUFFER, shape.vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float),
                 data.data(), GL_STATIC_DRAW);

    // pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // nor
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    shape.model = M;
}

// ======================================================================
// initializeGL
// ======================================================================

void Realtime::initializeGL() {
    m_devicePixelRatio = devicePixelRatio();
    m_timer = startTimer(16);
    m_elapsedTimer.start();

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Error while initializing GL: "
                  << glewGetErrorString(err) << std::endl;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glViewport(0, 0,
               width()  * m_devicePixelRatio,
               height() * m_devicePixelRatio);

    // Store Qt's default FBO, NOT a magic "3"
    m_defaultFBO = defaultFramebufferObject();

    // Cache screen size
    m_screen_width  = width()  * m_devicePixelRatio;
    m_screen_height = height() * m_devicePixelRatio;

    // --- Main (Phong + shadow) shader ---
    m_shader = ShaderLoader::createShaderProgram(
        ":/resources/shaders/default.vert",
        ":/resources/shaders/default.frag"
        );

    // --- Texture & shadow shaders (used by your FBO/shadow passes) ---
    m_texture_shader = ShaderLoader::createShaderProgram(
        ":/resources/shaders/texture.vert",
        ":/resources/shaders/texture.frag"
        );
    m_shadow_shader = ShaderLoader::createShaderProgram(
        ":/resources/shaders/shadowmap.vert",
        ":/resources/shaders/shadowmap.frag"
        );

    // --- HDR pipeline, resolve into Qt's default FBO ---
    m_hdr.init(width()  * m_devicePixelRatio,
               height() * m_devicePixelRatio,
               m_defaultFBO);

    // --- Your screen-space FBO used by paintTexture ---
    initializeFBO();

    // --- Shadow-map FBO (depth-only) ---
    initializeShadowFBO();

    // --- Camera trace & path ---
    m_camTrace.init();
    initCameraPath();

    // Load initial scene (also sets up shadow depth textures)
    sceneChanged();
}

// ======================================================================
// Lighting + material uniforms helper
// ======================================================================

void Realtime::shader(const RenderShapeData& shape,
                      const std::vector<SceneLightData>& lights)
{
    SceneGlobalData global   = m_renderData.globalData;
    SceneMaterial   material = shape.primitive.material;

    // *** CRUCIAL: match the uniform names in default.frag ***
    glm::vec3 Ka = global.ka * glm::vec3(material.cAmbient);
    glm::vec3 Kd = global.kd * glm::vec3(material.cDiffuse);
    glm::vec3 Ks = global.ks * glm::vec3(material.cSpecular);

    // MATCH default.frag: ka, kd, ks
    glUniform3fv(glGetUniformLocation(m_shader, "ka"), 1, &Ka[0]);
    glUniform3fv(glGetUniformLocation(m_shader, "kd"), 1, &Kd[0]);
    glUniform3fv(glGetUniformLocation(m_shader, "ks"), 1, &Ks[0]);
    glUniform1f(glGetUniformLocation(m_shader, "shininess"),
                material.shininess);

    // Upload lights
    int n = std::min<int>(lights.size(), numLights);
    for (int i = 0; i < n; i++) {
        const SceneLightData &light = lights[i];

        std::string base = "lights[" + std::to_string(i) + "].";

        glUniform1i(glGetUniformLocation(m_shader, (base + "type").c_str()),
                    int(light.type));

        glm::vec3 color = glm::vec3(light.color);
        glUniform3fv(glGetUniformLocation(m_shader, (base + "color").c_str()),
                     1, &color[0]);

        glm::vec3 pos = glm::vec3(light.pos);
        glm::vec3 dir = glm::normalize(glm::vec3(light.dir));
        glUniform3fv(glGetUniformLocation(m_shader, (base + "pos").c_str()),
                     1, &pos[0]);
        glUniform3fv(glGetUniformLocation(m_shader, (base + "dir").c_str()),
                     1, &dir[0]);

        glm::vec3 function = glm::vec3(light.function);
        glUniform3fv(glGetUniformLocation(m_shader, (base + "function").c_str()),
                     1, &function[0]);

        glUniform1f(glGetUniformLocation(m_shader, (base + "angle").c_str()),
                    light.angle);
        glUniform1f(glGetUniformLocation(m_shader, (base + "penumbra").c_str()),
                    light.penumbra);
    }
}


// ======================================================================
// paintGeometry  (Phong + optional soft shadows)
// ======================================================================

void Realtime::paintGeometry() {
    glUseProgram(m_shader);

    int fbWidth  = width()  * m_devicePixelRatio;
    int fbHeight = height() * m_devicePixelRatio;
    float aspect = float(fbWidth) / float(fbHeight);

    // Use the same camera that timer/mouse update & camera trace use
    glm::mat4 view = m_camera.getViewMatrix();
    glm::mat4 proj = m_camera.getProjectionMatrix(
        aspect,
        settings.nearPlane,
        settings.farPlane
        );

    m_view = view;
    m_proj = proj;

    glUniformMatrix4fv(glGetUniformLocation(m_shader, "view"),
                       1, GL_FALSE, &m_view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "proj"),
                       1, GL_FALSE, &m_proj[0][0]);

    glm::vec3 camPos = glm::vec3(m_camera.pos);
    glUniform3fv(glGetUniformLocation(m_shader, "cameraPos"),
                 1, glm::value_ptr(camPos));

    // ------------------------------------------------------------------
    // Shadow uniforms & binding are toggled by settings.extraCredit4
    // extraCredit4 == 0  -> behave like original implementation (no depth shadows)
    // extraCredit4 != 0  -> use depth maps + soft shadows
    // ------------------------------------------------------------------
    bool useDepthShadows = (settings.extraCredit4 != 0);

    glUniform1i(glGetUniformLocation(m_shader, "shadowSize"),
                useDepthShadows ? m_shadow_size : 0);
    glUniform1i(glGetUniformLocation(m_shader, "softShadows"),
                useDepthShadows ? 1 : 0);

    int nLights = std::min<int>(numLights, m_renderData.lights.size());
    glUniform1i(glGetUniformLocation(m_shader, "numLights"), nLights);

    if (useDepthShadows) {
        for (int i = 0; i < nLights; i++) {
            std::string mvpName = "lightMVPs[" + std::to_string(i) + "]";
            glUniformMatrix4fv(glGetUniformLocation(m_shader, mvpName.c_str()),
                               1, GL_FALSE, &m_light_MVPs[i][0][0]);

            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, m_shadow_maps[i]);
            std::string smName = "shadowMaps[" + std::to_string(i) + "]";
            glUniform1i(glGetUniformLocation(m_shader, smName.c_str()), i);
        }
    }
    // If !useDepthShadows, we do NOT bind shadow maps.
    // Fragment shader should skip any sampling based on softShadows/shadowSize.

    // ==== Draw analytic primitives via m_objects ====
    size_t objIdx = 0;
    for (const RenderShapeData &shape : m_renderData.shapes) {
        PrimitiveType t = shape.primitive.type;

        bool isAnalytic =
            (t == PrimitiveType::PRIMITIVE_CUBE     ||
             t == PrimitiveType::PRIMITIVE_CONE     ||
             t == PrimitiveType::PRIMITIVE_SPHERE   ||
             t == PrimitiveType::PRIMITIVE_CYLINDER);

        if (!isAnalytic) {
            continue;
        }

        if (objIdx >= m_objects.size()) break;
        GLShape &obj = m_objects[objIdx++];

        // Model / normal matrices
        m_model       = obj.model;
        m_normalModel = glm::transpose(glm::inverse(obj.model));
        glUniformMatrix4fv(glGetUniformLocation(m_shader, "model"),
                           1, GL_FALSE, &m_model[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(m_shader, "normalModel"),
                           1, GL_FALSE, &m_normalModel[0][0]);

        // Upload material & lights
        shader(shape, m_renderData.lights);

        glBindVertexArray(obj.vao);
        glDrawArrays(obj.mode, 0, obj.count);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

// ======================================================================
// paintGL  (HDR + optional soft shadows + camera trace)
// ======================================================================

void Realtime::paintGL() {
    bool useHDR    = settings.extraCredit1;
    bool showTrace = settings.extraCredit2;

    int fbWidth  = width()  * m_devicePixelRatio;
    int fbHeight = height() * m_devicePixelRatio;

    // Keep "teammate-style" camera values in sync with Camera object
    m_camPos  = glm::vec3(m_camera.pos);
    m_camLook = glm::vec3(m_camera.look);
    m_camUp   = glm::vec3(m_camera.up);

    // ------------------------------------------------------------------
    // 1) Shadow pass ONLY when using depth-map soft shadows
    //    (settings.extraCredit4 == 1).
    //    When off, we entirely skip the depth shadow pipeline and fall
    //    back to "original" shading.
    // ------------------------------------------------------------------
    if (settings.extraCredit4) {
        computeLightMVPs();
        paintShadows();
    }

    // 2) Main scene pass (HDR FBO or your color+depth FBO)
    if (useHDR) {
        m_hdr.beginRender();
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    }

    glViewport(0, 0, fbWidth, fbHeight);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    paintGeometry();

    // View/proj from Camera for the camera trace
    glm::mat4 view = m_camera.getViewMatrix();
    float aspect = float(fbWidth) / float(fbHeight);
    glm::mat4 proj = m_camera.getProjectionMatrix(
        aspect, settings.nearPlane, settings.farPlane
        );

    if (showTrace) {
        m_camTrace.draw(view, proj);
    }

    // 3) Resolve to screen
    if (useHDR) {
        m_hdr.endRender();

        glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
        glViewport(0, 0, fbWidth, fbHeight);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);

        m_hdr.drawTonemap(width(), height());
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
        glViewport(0, 0, fbWidth, fbHeight);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);

        // Your FBO blit (uses m_texture_shader + fullscreen quad)
        paintTexture(m_fbo_texture, m_fbo_depth);
    }
}

// ======================================================================
// resizeGL
// ======================================================================

void Realtime::resizeGL(int w, int h) {
    m_devicePixelRatio = devicePixelRatio();

    int fbWidth  = w * m_devicePixelRatio;
    int fbHeight = h * m_devicePixelRatio;

    glViewport(0, 0, fbWidth, fbHeight);

    // Default FBO can change on resize on some platforms
    m_defaultFBO = defaultFramebufferObject();

    // Rebuild your color+depth FBO
    if (m_fbo_texture) glDeleteTextures(1, &m_fbo_texture);
    if (m_fbo_depth)   glDeleteTextures(1, &m_fbo_depth);
    if (m_fbo)         glDeleteFramebuffers(1, &m_fbo);

    m_fbo_width  = fbWidth;
    m_fbo_height = fbHeight;
    makeFBO();

    // Resize HDR FBO as well
    m_hdr.resize(fbWidth, fbHeight, m_defaultFBO);
}

// ======================================================================
// Scene + settings
// ======================================================================

void Realtime::sceneChanged() {
    if (settings.sceneFilePath.empty()) return;

    RenderData newData;
    if (!SceneParser::parse(settings.sceneFilePath, newData)) {
        std::cerr << "ERROR: Failed to load scene\n";
        return;
    }

    m_renderData = std::move(newData);

    // Camera object
    m_camera = Camera(m_renderData.cameraData);

    // Also sync teammate-style camera representation
    m_camPos  = glm::vec3(m_renderData.cameraData.pos);
    m_camLook = glm::vec3(m_renderData.cameraData.look);
    m_camUp   = glm::normalize(glm::vec3(m_renderData.cameraData.up));

    // Rebuild analytic primitive VAOs
    rebuildScene();

    // Re-init shadow depth textures based on lights
    initializeShadowDepths();

    update();
}

void Realtime::settingsChanged() {
    rebuildScene();
    update();
}

void Realtime::rebuildScene() {
    for (auto &o : m_objects) o.destroy();
    m_objects.clear();

    if (m_renderData.shapes.empty()) return;

    int p1 = settings.shapeParameter1;
    int p2 = settings.shapeParameter2;

    for (const auto &s : m_renderData.shapes) {
        std::vector<float> data;

        switch (s.primitive.type) {
        case PrimitiveType::PRIMITIVE_CUBE: {
            Cube c; c.updateParams(p1); data = c.generateShape(); break;
        }
        case PrimitiveType::PRIMITIVE_CONE: {
            Cone c; c.updateParams(p1, p2); data = c.generateShape(); break;
        }
        case PrimitiveType::PRIMITIVE_SPHERE: {
            Sphere c; c.updateParams(p1, p2); data = c.generateShape(); break;
        }
        case PrimitiveType::PRIMITIVE_CYLINDER: {
            Cylinder c; c.updateParams(p1, p2); data = c.generateShape(); break;
        }
        default:
            // PRIMITIVE_MESH handled elsewhere via m_meshes
            continue;
        }

        GLShape shape;
        buildShape(shape, data, s.ctm);
        m_objects.push_back(shape);
    }
}

// ======================================================================
// Input Handling
// ======================================================================

void Realtime::keyPressEvent(QKeyEvent *e) {
    m_keyMap[Qt::Key(e->key())] = true;
}
void Realtime::keyReleaseEvent(QKeyEvent *e) {
    m_keyMap[Qt::Key(e->key())] = false;
}

void Realtime::mousePressEvent(QMouseEvent *e) {
    if (e->buttons() & Qt::LeftButton) {
        m_mouseDown = true;
        m_prev_mouse_pos = { e->position().x(), e->position().y() };
    }
}

void Realtime::mouseReleaseEvent(QMouseEvent *e) {
    if (!(e->buttons() & Qt::LeftButton)) {
        m_mouseDown = false;
    }
}

void Realtime::mouseMoveEvent(QMouseEvent *e) {
    if (!m_mouseDown) return;

    float dx = e->position().x() - m_prev_mouse_pos.x;
    float dy = e->position().y() - m_prev_mouse_pos.y;
    m_prev_mouse_pos = { e->position().x(), e->position().y() };

    if (!settings.extraCredit3) {
        m_camera.rotateYawPitch(-dx * 0.003f, -dy * 0.003f);
    }

    update();
}

// ======================================================================
// Timer (WASD movement + path-following + trace)
// ======================================================================

void Realtime::timerEvent(QTimerEvent*) {
    float dt = m_elapsedTimer.elapsed() * 0.001f;
    m_elapsedTimer.restart();

    bool pathMode = settings.extraCredit3;

    if (pathMode) {
        if (!m_followingPath) {
            // Entering path mode: build canonical path
            initCameraPath();

            // Anchor path start to current camera position
            glm::vec3 camStart = glm::vec3(m_camera.pos);
            if (!m_camPath.pts.empty()) {
                glm::vec3 pathStart = m_camPath.pts.front();
                glm::vec3 offset = camStart - pathStart;
                for (auto &p : m_camPath.pts) {
                    p += offset;
                }
            }

            m_camPath.reset();
            m_followingPath = true;
        }

        bool finished = false;
        glm::vec3 P = m_camPath.sample(dt, finished);
        m_camera.pos = glm::vec4(P, 1.0f);

        // Simple "look at origin" orientation for now
        m_camera.lookAtPoint(glm::vec3(0.0f));
    } else {
        m_followingPath = false;

        float s = 5.f;
        if (m_keyMap[Qt::Key_W])       m_camera.moveForward( s * dt);
        if (m_keyMap[Qt::Key_S])       m_camera.moveForward(-s * dt);
        if (m_keyMap[Qt::Key_A])       m_camera.moveRight  (-s * dt);
        if (m_keyMap[Qt::Key_D])       m_camera.moveRight  ( s * dt);
        if (m_keyMap[Qt::Key_Space])   m_camera.moveUp     ( s * dt);
        if (m_keyMap[Qt::Key_Control]) m_camera.moveUp     (-s * dt);
    }

    // Keep teammate-style camera representation in sync
    m_camPos  = glm::vec3(m_camera.pos);
    m_camLook = glm::vec3(m_camera.look);
    m_camUp   = glm::vec3(m_camera.up);

    // Trace system
    if (settings.extraCredit2) {
        m_camTrace.update(dt, m_camPos);
    } else {
        m_camTrace.reset();
    }

    update();
}

// ======================================================================
// Save viewport
// ======================================================================

void Realtime::saveViewportImage(std::string filePath) {
    makeCurrent();

    GLint vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
    int w = vp[2], h = vp[3];

    GLuint fbo, tex, rbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, tex, 0);

    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer incomplete\n";
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    paintGL();

    std::vector<unsigned char> pixels(w * h * 3);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    QImage img(pixels.data(), w, h, QImage::Format_RGB888);
    img.mirrored().save(QString::fromStdString(filePath));

    glDeleteTextures(1, &tex);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);
}
