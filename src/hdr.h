#pragma once

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include <GL/glew.h>
#include "utils/shaderloader.h"

class HDR {
public:
    HDR() = default;

    // Pass Qt's default FBO from Realtime::initializeGL / resizeGL
    void init(int width, int height, GLuint defaultFBO);
    void resize(int width, int height, GLuint defaultFBO);

    void beginRender();   // bind HDR FBO & clear
    void endRender();     // bind default Qt FBO again
    void drawTonemap(int windowWidth, int windowHeight);   // fullscreen quad → tonemap HDR → default FBO

    void destroy();

    // Control brightness of HDR output (1.0 is a good starting point)
    void setExposure(float e) { m_exposure = e; }

private:
    GLuint m_fbo        = 0;
    GLuint m_color      = 0;
    GLuint m_rbo        = 0;

    GLuint m_quadVAO    = 0;
    GLuint m_quadVBO    = 0;

    GLuint m_toneShader = 0;
    GLint  m_uHdrBuffer = -1;
    GLint  m_uExposure  = -1;

    int    m_width      = 0;
    int    m_height     = 0;
    GLuint m_defaultFBO = 0;

    float  m_exposure   = 1.0f; // HDR exposure

    void buildFullScreenQuad();
    GLuint buildShader();
};
