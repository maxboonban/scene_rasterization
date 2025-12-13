#include "hdr.h"
#include <iostream>

// Fullscreen quad vertices (NDC positions + UVs)
static const float FSQ[] = {
    -1, -1,  0, 0,
    1, -1,  1, 0,
    1,  1,  1, 1,
    -1, -1,  0, 0,
    1,  1,  1, 1,
    -1,  1,  0, 1
};

// ====================== Inline Shaders ======================

static const char* TM_VERT = R"(
#version 330 core
layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 uvIn;
out vec2 uv;
void main() {
    uv = uvIn;
    gl_Position = vec4(pos, 0.0, 1.0);
}
)";

// Exposure + filmic-ish exponential tonemap in *linear* space, then gamma
static const char* TM_FRAG = R"(
#version 330 core
in vec2 uv;
out vec4 fragColor;

uniform sampler2D hdrBuffer;   // HDR color buffer (linear)
uniform float exposure;        // controls overall brightness

void main() {
    vec3 hdr = texture(hdrBuffer, uv).rgb;
    hdr = max(hdr, vec3(0.0));          // avoid negative values

    // Simple exponential tonemap: 1 - exp(-x * exposure)
    vec3 mapped = vec3(1.0) - exp(-hdr * exposure);

    // Gamma correction to sRGB
    mapped = pow(mapped, vec3(1.0 / 2.2));

    fragColor = vec4(mapped, 1.0);
}
)";

// ====================== Helpers ======================

GLuint HDR::buildShader() {
    auto make = [&](GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = GL_FALSE;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetShaderInfoLog(s, 1024, nullptr, log);
            std::cerr << "Shader compile error:\n" << log << "\n";
        }
        return s;
    };

    GLuint v = make(GL_VERTEX_SHADER, TM_VERT);
    GLuint f = make(GL_FRAGMENT_SHADER, TM_FRAG);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, v);
    glAttachShader(prog, f);
    glLinkProgram(prog);

    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, 1024, nullptr, log);
        std::cerr << "Program link error:\n" << log << "\n";
    }

    glDeleteShader(v);
    glDeleteShader(f);

    // Cache uniform locations
    glUseProgram(prog);
    m_uHdrBuffer = glGetUniformLocation(prog, "hdrBuffer");
    m_uExposure  = glGetUniformLocation(prog, "exposure");
    glUseProgram(0);

    return prog;
}

void HDR::buildFullScreenQuad() {
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);

    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(FSQ), FSQ, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ====================== Core API ======================

void HDR::init(int width, int height, GLuint defaultFBO) {
    destroy();

    m_width      = width;
    m_height     = height;
    m_defaultFBO = defaultFBO;

    if (!m_toneShader)
        m_toneShader = buildShader();
    if (!m_quadVAO)
        buildFullScreenQuad();

    // ---------- HDR FBO ----------
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // Floating point color buffer (true HDR storage)
    glGenTextures(1, &m_color);
    glBindTexture(GL_TEXTURE_2D, m_color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F,
                 m_width, m_height, 0,
                 GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, m_color, 0);

    // Depth-stencil renderbuffer
    glGenRenderbuffers(1, &m_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
                          m_width, m_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, m_rbo);

    // Make sure we are drawing to the color attachment
    GLenum drawBuf = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuf);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "HDR FBO incomplete!\n";
    }

    // Restore Qt's default FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
}

void HDR::resize(int width, int height, GLuint defaultFBO) {
    init(width, height, defaultFBO);
}

void HDR::beginRender() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void HDR::endRender() {
    // Go back to the FBO Qt actually displays
    glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
}

void HDR::drawTonemap(int windowWidth, int windowHeight) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
    glViewport(0, 0, windowWidth, windowHeight);

    glDisable(GL_DEPTH_TEST);
    glUseProgram(m_toneShader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_color);

    if (m_uHdrBuffer >= 0) glUniform1i(m_uHdrBuffer, 0);
    if (m_uExposure >= 0)  glUniform1f(m_uExposure, m_exposure);

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    glEnable(GL_DEPTH_TEST);
}

void HDR::destroy() {
    if (m_color) glDeleteTextures(1, &m_color);
    if (m_rbo)   glDeleteRenderbuffers(1, &m_rbo);
    if (m_fbo)   glDeleteFramebuffers(1, &m_fbo);
    m_fbo = m_color = m_rbo = 0;

    if (m_quadVBO) glDeleteBuffers(1, &m_quadVBO);
    if (m_quadVAO) glDeleteVertexArrays(1, &m_quadVAO);
    m_quadVBO = m_quadVAO = 0;

    if (m_toneShader) glDeleteProgram(m_toneShader);
    m_toneShader = 0;

    m_uHdrBuffer = -1;
    m_uExposure  = -1;
}
