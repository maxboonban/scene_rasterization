#include "realtime.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include "settings.h"
#include <glm/gtx/string_cast.hpp>

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
    glDeleteProgram(m_shader);
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);

    glDeleteProgram(m_durand);
    glDeleteVertexArrays(1, &m_fullscreen_vao);
    glDeleteTextures(1, &m_fbo_texture);
    glDeleteRenderbuffers(1, &m_fbo_renderbuffer);
    glDeleteFramebuffers(1, &m_fbo);

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
    glClearColor(0, 0, 0, 1);

    m_shader = ShaderLoader::createShaderProgram(":/resources/shaders/default.vert", ":/resources/shaders/default.frag");
    m_durand = ShaderLoader::createShaderProgram(":/resources/shaders/durand.vert", ":/resources/shaders/durand.frag");
    
    glUseProgram(m_durand);
    GLint loc = glGetUniformLocation(m_durand, "sampler");
    glUniform1i(loc, 0);

    fullscreen_quad_data =
    {
            -1.0f,  1.0f, 0.0f,
            0.0f,  1.0f,
            -1.0f, -1.0f, 0.0f,
            0.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
            1.0f, 0.0f,
            1.0f,  1.0f, 0.0f,
            1.0f, 1.0f,
            -1.0f,  1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, -1.0f, 0.0f,
            1.0f, 0.0f
    };

    glGenVertexArrays(1, &m_fullscreen_vao);

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    makeFBO();

    regenerateTris(settings.shapeParameter1, settings.shapeParameter2);

    t = 0;
    M0 = glm::mat4(0.743294, 0.224795, -0.630064, 0.000000,-0.000000, 0.941850, 0.336034, 0.000000,0.668965, -0.249772, 0.700071, 0.000000,0.000000, 0.000000, -3.571064, 1.000000);
    M1 = glm::mat4(0.950567, 0.128616, -0.282630, 0.000000,-0.071763, 0.976539, 0.203032, 0.000000,0.302112, -0.172714, 0.937496, 0.000000,1.163417, -0.455088, -3.467533, 1.000000);
    M2 = glm::mat4(0.705314, 0.012754, 0.708780, 0.000000,0.046201, 0.996886, -0.063913, 0.000000,-0.707388, 0.077825, 0.702528, 0.000000,-5.224429, -1.402385, -3.754068, 1.000000);
    M3 = glm::mat4(0.741220, -0.190246, 0.643739, 0.000000,-0.016422, 0.953571, 0.300720, 0.000000,-0.671062, -0.233471, 0.703681, 0.000000,-4.556030, -3.016252, -3.574254, 1.000000);
}

std::vector<GLfloat> Realtime::selectPrimitiveTris(const RenderShapeData &shape)
{
    switch (shape.primitive.type)
    {
    case PrimitiveType::PRIMITIVE_CUBE:
        return cubeData;
    case PrimitiveType::PRIMITIVE_CONE:
        return coneData;
    case PrimitiveType::PRIMITIVE_CYLINDER:
        return cylinderData;
    case PrimitiveType::PRIMITIVE_SPHERE:
        return sphereData;
    case PrimitiveType::PRIMITIVE_MESH:
        return shape.triData;
    }
}

void Realtime::passUniformArgs(GLuint shader, const RenderShapeData &shape)
{       
    glm::mat4 invT = glm::transpose(glm::inverse(shape.ctm));
    glUniformMatrix4fv(glGetUniformLocation(shader, "modelMat"), 1, GL_FALSE, &shape.ctm[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader, "modelMatInvT"), 1, GL_FALSE, &invT[0][0]);

    glUniform4fv(glGetUniformLocation(shader, "ca"), 1, &shape.primitive.material.cAmbient[0]);
    glUniform4fv(glGetUniformLocation(shader, "cd"), 1, &shape.primitive.material.cDiffuse[0]);
    glUniform4fv(glGetUniformLocation(shader, "cs"), 1, &shape.primitive.material.cSpecular[0]);
    glUniform1f(glGetUniformLocation(shader, "n"), shape.primitive.material.shininess);
}

void Realtime::makeFBO(){
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &m_fbo_texture);
    glBindTexture(GL_TEXTURE_2D, m_fbo_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenRenderbuffers(1, &m_fbo_renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_fbo_renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fbo_texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_fbo_renderbuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
}


void Realtime::paintTexture(GLuint texture){
    glUseProgram(m_durand);
    GLint loc = glGetUniformLocation(m_durand, "sampler");
    glUniform1i(loc, 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, fullscreen_quad_data.size()*sizeof(GLfloat), fullscreen_quad_data.data(), GL_STATIC_DRAW);

    glBindVertexArray(m_fullscreen_vao);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), reinterpret_cast<void *>(0 * sizeof(GLfloat)));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void Realtime::paintGL() {
    // Students: anything requiring OpenGL calls every frame should be done here
    glUseProgram(m_shader);

    // t += 0.01;
    // camera.cubicBezier(M0, M1, M2, M3, t);

    glm::mat4 m_view = camera.getViewMatrix();
    glm::mat4 m_proj = camera.getProjMatrix();

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (const RenderShapeData &shape : sceneData.shapes)
    {
        std::vector<GLfloat> tris = selectPrimitiveTris(shape);
        passUniformArgs(m_shader, shape);

        glUniform1i(glGetUniformLocation(m_shader, "numLights"), sceneData.lights.size());
        for (int i = 0; i < fmin(8, sceneData.lights.size()); i++)
        {
            std::string lightPos = "lightPos[" + std::to_string(i) + "]";
            GLint loc = glGetUniformLocation(m_shader, lightPos.c_str());
            glUniform4fv(loc, 1, &sceneData.lights[i].pos[0]);

            std::string lightDir = "lightDir[" + std::to_string(i) + "]";
            loc = glGetUniformLocation(m_shader, lightDir.c_str());
            glUniform4fv(loc, 1, &sceneData.lights[i].dir[0]);

            std::string lightColor = "lightColor[" + std::to_string(i) + "]";
            loc = glGetUniformLocation(m_shader, lightColor.c_str());
            glUniform4fv(loc, 1, &sceneData.lights[i].color[0]);

            std::string lightFunc = "lightFunc[" + std::to_string(i) + "]";
            loc = glGetUniformLocation(m_shader, lightFunc.c_str());
            glUniform3fv(loc, 1, &sceneData.lights[i].function[0]);

            std::string lightAngle = "lightAngle[" + std::to_string(i) + "]";
            loc = glGetUniformLocation(m_shader, lightAngle.c_str());
            glUniform1f(loc, sceneData.lights[i].angle);

            std::string lightPenumbra = "lightPenumbra[" + std::to_string(i) + "]";
            loc = glGetUniformLocation(m_shader, lightPenumbra.c_str());
            glUniform1f(loc, sceneData.lights[i].penumbra);

            std::string lightType = "lightType[" + std::to_string(i) + "]";
            loc = glGetUniformLocation(m_shader, lightType.c_str());
            glUniform1i(loc, (int) sceneData.lights[i].type);
        }

        glm::vec4 cameraPosWorld = glm::inverse(m_view) * glm::vec4(0, 0, 0, 1);
        glUniform1f(glGetUniformLocation(m_shader, "ka"), sceneData.globalData.ka);
        glUniform1f(glGetUniformLocation(m_shader, "kd"), sceneData.globalData.kd);
        glUniform1f(glGetUniformLocation(m_shader, "ks"), sceneData.globalData.ks);
        glUniformMatrix4fv(glGetUniformLocation(m_shader, "viewMat"), 1, GL_FALSE, &m_view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(m_shader, "projMat"), 1, GL_FALSE, &m_proj[0][0]);
        glUniform4fv(glGetUniformLocation(m_shader, "cameraPos"), 1, &cameraPosWorld[0]);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, tris.size() * sizeof(GLfloat), &tris.front(), GL_STATIC_DRAW);

        glBindVertexArray(m_vao);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void *>(0 * sizeof(GLfloat)));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
        glDrawArrays(GL_TRIANGLES, 0, tris.size() / 6);

    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    paintTexture(m_fbo_texture);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void Realtime::resizeGL(int w, int h) {
    // Tells OpenGL how big the screen is
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);

    // Students: anything requiring OpenGL calls when the program starts should be done here
    glDeleteTextures(1, &m_fbo_texture);
    glDeleteRenderbuffers(1, &m_fbo_renderbuffer);
    glDeleteFramebuffers(1, &m_fbo);
    makeFBO();
}

void Realtime::sceneChanged() {
    sceneData.lights.clear();
    sceneData.shapes.clear();
    bool success = SceneParser::parse(settings.sceneFilePath, sceneData);

    if (!success) {
        std::cerr << "Error loading scene: \"" << settings.sceneFilePath << "\"" << std::endl;
    }
    else std::cout << "Parsed scene: \"" << settings.sceneFilePath << "\"" << std::endl;
    camera.update(sceneData.cameraData, settings.nearPlane, settings.farPlane, size().width(), size().height());
    regenerateTris(settings.shapeParameter1, settings.shapeParameter2);
    update(); // asks for a PaintGL() call to occur
    t = 0;
}

void Realtime::settingsChanged() {
    regenerateTris(settings.shapeParameter1, settings.shapeParameter2);
    camera.update(sceneData.cameraData, settings.nearPlane, settings.farPlane, size().width(), size().height());
    update(); // asks for a PaintGL() call to occur
}

void Realtime::regenerateTris(int param_1, int param_2) {
    coneData.clear();
    Cone::makeCone(param_1, param_2, coneData);

    cubeData.clear();
    Cube::makeCube(param_1, param_2, cubeData);

    cylinderData.clear();
    Cylinder::makeCylinder(param_1, param_2, cylinderData);

    sphereData.clear();
    Sphere::makeSphere(param_1, param_2, sphereData);
};

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

        float angleX = 180.f * deltaX / size().width();
        float angleY = 180.f * deltaY / size().height();

        glm::vec3 right = glm::cross(glm::vec3(sceneData.cameraData.look), glm::vec3(sceneData.cameraData.up));
        glm::mat4 rotation = Transforms::getRotationMatrix(angleX, glm::vec3(0,1,0)) * Transforms::getRotationMatrix(angleY, right);
        
        camera.look = glm::mat3(rotation) * camera.look;
        camera.up = glm::mat3(rotation) * camera.up;

        camera.computeViewMatrix();

        update(); // asks for a PaintGL() call to occur
    }
}

void Realtime::timerEvent(QTimerEvent *event) {
    int elapsedms   = m_elapsedTimer.elapsed();
    float deltaTime = elapsedms * 0.001f;
    m_elapsedTimer.restart();

    // Use deltaTime and m_keyMap here to move around
    if (m_keyMap[Qt::Key_W])
    {
        glm::vec4 look = sceneData.cameraData.look;
        look = glm::normalize(look) * 5.f * deltaTime;
        camera.worldPos = Transforms::getTranslationMatrix(look.x, look.y, look.z) * camera.worldPos;
    };

    if (m_keyMap[Qt::Key_A])
    {
        glm::vec3 left = -glm::cross(glm::vec3(sceneData.cameraData.look), glm::vec3(sceneData.cameraData.up));
        left = glm::normalize(left) * 5.f * deltaTime;
        camera.worldPos = Transforms::getTranslationMatrix(left.x, left.y, left.z) * camera.worldPos;
    };

    if (m_keyMap[Qt::Key_S])
    {
        glm::vec4 look = -sceneData.cameraData.look;
        look = glm::normalize(look) * 5.f * deltaTime;
        camera.worldPos = Transforms::getTranslationMatrix(look.x, look.y, look.z) * camera.worldPos;
    };

    if (m_keyMap[Qt::Key_D])
    {
        glm::vec3 right = glm::cross(glm::vec3(sceneData.cameraData.look), glm::vec3(sceneData.cameraData.up));
        right = glm::normalize(right) * 5.f * deltaTime;
        camera.worldPos = Transforms::getTranslationMatrix(right.x, right.y, right.z) * camera.worldPos;
    };

    if (m_keyMap[Qt::Key_Space])
    {
        glm::vec3 up = glm::vec3(0, 1, 0);
        up = glm::normalize(up) * 5.f * deltaTime;
        camera.worldPos = Transforms::getTranslationMatrix(up.x, up.y, up.z) * camera.worldPos;
    };

    if (m_keyMap[Qt::Key_Control])
    {
        glm::vec3 down = glm::vec3(0, -1, 0);
        down = glm::normalize(down) * 5.f * deltaTime;
        camera.worldPos = Transforms::getTranslationMatrix(down.x, down.y, down.z) * camera.worldPos;
    };

    camera.computeViewMatrix();

    update(); // asks for a PaintGL() call to occur
}

// DO NOT EDIT
void Realtime::saveViewportImage(std::string filePath) {
    // Make sure we have the right context and everything has been drawn
    makeCurrent();

    int fixedWidth = size().width() * m_devicePixelRatio;
    int fixedHeight = size().height() * m_devicePixelRatio;

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
