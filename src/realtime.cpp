#include "realtime.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include "settings.h"
#include "shaderloader.h"
#include "camera/camera.h"

// ================== Rendering the Scene!

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

    // If you must use this function, do not edit anything above this
}

void Realtime::finish() {
    killTimer(m_timer);
    this->makeCurrent();

    // Students: anything requiring OpenGL calls when the program exits should be done here
    // clean up memory
    for (const auto& pair : m_meshes) {
        glDeleteBuffers(1, &pair.second.vbo);
        glDeleteVertexArrays(1, &pair.second.vao);
    }
    glDeleteProgram(m_shader);
    // depth buffer memory
    glDeleteProgram(m_texture_shader);
    glDeleteVertexArrays(1, &m_fullscreen_vao);
    glDeleteBuffers(1, &m_fullscreen_vbo);
    glDeleteTextures(1, &m_fbo_texture);
    glDeleteTextures(1, &m_fbo_depth);
    glDeleteFramebuffers(1, &m_fbo);
    // shadow mapping memory
    glDeleteProgram(m_shadow_shader);
    glDeleteTextures(m_shadow_maps.size(), m_shadow_maps.data());
    glDeleteFramebuffers(1, &m_fbo_shadow);

    this->doneCurrent();
}

void Realtime::initializeGL() {
    m_devicePixelRatio = this->devicePixelRatio();

    m_timer = startTimer(1000/60);
    m_elapsedTimer.start();

    // Initializing GL.
    // GLEW (GL Extension Wrangler) provides access to OpenGL functions.
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Error while initializing GL: " << glewGetErrorString(err) << std::endl;
    }
    std::cout << "Initialized GL: Version " << glewGetString(GLEW_VERSION) << std::endl;

    // Allows OpenGL to draw objects appropriately on top of one another
    glEnable(GL_DEPTH_TEST);
    // Tells OpenGL to only draw the front face
    glEnable(GL_CULL_FACE);
    // Tells OpenGL how big the screen is
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);

    // Students: anything requiring OpenGL calls when the program starts should be done here

    m_defaultFBO = 3;
    m_screen_width = size().width() * m_devicePixelRatio;
    m_screen_height = size().height() * m_devicePixelRatio;

    // set clear color to black
    glClearColor(0, 0, 0, 1);

    // shaders
    m_shader = ShaderLoader::createShaderProgram("resources/shaders/default.vert",
                                                 "resources/shaders/default.frag");
    m_texture_shader = ShaderLoader::createShaderProgram("resources/shaders/texture.vert",
                                                         "resources/shaders/texture.frag");
    m_shadow_shader = ShaderLoader::createShaderProgram("resources/shaders/shadowmap.vert",
                                                        "resources/shaders/shadowmap.frag");

    initializeFBO();
    initializeShadowFBO();
}

void Realtime::paintGL() {
    // Students: anything requiring OpenGL calls every frame should be done here

    // // shadow mapping
    computeLightMVPs();
    paintShadows();

    // bind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    m_fbo_width = size().width()  * m_devicePixelRatio;
    m_fbo_height = size().height() * m_devicePixelRatio;
    glViewport(0, 0, m_fbo_width, m_fbo_height);

    // phong
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    paintGeometry();

    // bind default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);

    // clear the color buffers
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    // draw FBO color and depth attachment textures
    paintTexture(m_fbo_texture, m_fbo_depth);
}

void Realtime::resizeGL(int w, int h) {
    // Tells OpenGL how big the screen is
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);

    // Students: anything requiring OpenGL calls when the program starts should be done here
    glDeleteTextures(1, &m_fbo_texture);
    glDeleteTextures(1, &m_fbo_depth);
    glDeleteFramebuffers(1, &m_fbo);

    m_fbo_width = size().width() * m_devicePixelRatio;
    m_fbo_height = size().height() * m_devicePixelRatio;
    makeFBO();
}

void Realtime::sceneChanged() {
    // Parse new scene
    m_metaData = {};
    SceneParser::parse(settings.sceneFilePath, m_metaData);
    SceneCameraData camData = m_metaData.cameraData;
    m_camPos  = glm::vec3(camData.pos);
    m_camLook = glm::vec3(camData.look);
    m_camUp   = glm::normalize(glm::vec3(camData.up));

    // reinitialize shadow depths after loading scene
    initializeShadowDepths();

    update(); // asks for a PaintGL() call to occur
}

void Realtime::settingsChanged() {
    update(); // asks for a PaintGL() call to occur
}

// ================== Camera Movement!

void Realtime::keyPressEvent(QKeyEvent *event) {
    m_keyMap[Qt::Key(event->key())] = true;
}

void Realtime::keyReleaseEvent(QKeyEvent *event) {
    m_keyMap[Qt::Key(event->key())] = false;
}

void Realtime::mousePressEvent(QMouseEvent *event) {
    if (event->buttons().testFlag(Qt::LeftButton)) {
        m_mouseDown = true;
        m_prev_mouse_pos = glm::vec2(event->position().x(), event->position().y());
    }
}

void Realtime::mouseReleaseEvent(QMouseEvent *event) {
    if (!event->buttons().testFlag(Qt::LeftButton)) {
        m_mouseDown = false;
    }
}

void Realtime::mouseMoveEvent(QMouseEvent *event) {
    if (m_mouseDown) {
        int posX = event->position().x();
        int posY = event->position().y();
        int deltaX = posX - m_prev_mouse_pos.x;
        int deltaY = posY - m_prev_mouse_pos.y;
        m_prev_mouse_pos = glm::vec2(posX, posY);

        // Use deltaX and deltaY here to rotate
        m_angleX = 10.f * (float) deltaX / (float) size().width();
        m_angleY = 10.f * (float) deltaY / (float) size().height();
        updateMouseRotation();
    }
}

void Realtime::timerEvent(QTimerEvent *event) {
    int elapsedms   = m_elapsedTimer.elapsed();
    float deltaTime = elapsedms * 0.001f;
    m_elapsedTimer.restart();

    // forward/back and left/right translation directions
    glm::vec3 forward = glm::normalize(m_camLook);
    glm::vec3 right = glm::normalize(glm::cross(m_camLook, m_camUp));

    if (m_keyMap[Qt::Key_W]) { m_camPos += forward * 5.f * deltaTime; }
    if (m_keyMap[Qt::Key_S]) { m_camPos -= forward * 5.f * deltaTime; }
    if (m_keyMap[Qt::Key_A]) { m_camPos -= right * 5.f * deltaTime; }
    if (m_keyMap[Qt::Key_D]) { m_camPos += right * 5.f * deltaTime; }
    if (m_keyMap[Qt::Key_Space]) { m_camPos += glm::vec3(0.f, 1.f, 0.f) * 5.f * deltaTime; }
    if (m_keyMap[Qt::Key_Control]) { m_camPos -= glm::vec3(0.f, 1.f, 0.f) * 5.f * deltaTime; }

    update(); // asks for a PaintGL() call to occur
}

// DO NOT EDIT
void Realtime::saveViewportImage(std::string filePath) {
    // Make sure we have the right context and everything has been drawn
    makeCurrent();

    int fixedWidth = 1024;
    int fixedHeight = 768;

    // Create Frame Buffer
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create a color attachment texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fixedWidth, fixedHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    // Optional: Create a depth buffer if your rendering uses depth testing
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fixedWidth, fixedHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    // Render to the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, fixedWidth, fixedHeight);

    // Clear and render your scene here
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    paintGL();

    // Read pixels from framebuffer
    std::vector<unsigned char> pixels(fixedWidth * fixedHeight * 3);
    glReadPixels(0, 0, fixedWidth, fixedHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // Unbind the framebuffer to return to default rendering to the screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Convert to QImage
    QImage image(pixels.data(), fixedWidth, fixedHeight, QImage::Format_RGB888);
    QImage flippedImage = image.mirrored(); // Flip the image vertically

    // Save to file using Qt
    QString qFilePath = QString::fromStdString(filePath);
    if (!flippedImage.save(qFilePath)) {
        std::cerr << "Failed to save image to " << filePath << std::endl;
    }

    // Clean up
    glDeleteTextures(1, &texture);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);
}
