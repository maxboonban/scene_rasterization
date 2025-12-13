#include "cameratrace.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <iostream>

// --------------------- Inline shaders ---------------------

static const char *TRACE_VERT = R"(
#version 330 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in float inAlpha;

uniform mat4 view;
uniform mat4 proj;

out float vAlpha;

void main() {
    vAlpha = inAlpha;
    gl_Position = proj * view * vec4(inPos, 1.0);
}
)";

static const char *TRACE_FRAG = R"(
#version 330 core

in float vAlpha;
out vec4 fragColor;

void main() {
    // Cyan-ish trail, fading with alpha
    vec3 baseColor = vec3(0.1, 0.9, 1.0);
    fragColor = vec4(baseColor * vAlpha, vAlpha);
}
)";

// --------------------- Helper: shader build ---------------------

static GLuint compileShader(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint status = GL_FALSE;
    glGetShaderiv(s, GL_COMPILE_STATUS, &status);
    if (!status) {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len);
        glGetShaderInfoLog(s, len, nullptr, log.data());
        std::cerr << "CameraTrace shader compile error:\n" << log.data() << std::endl;
    }
    return s;
}

GLuint CameraTrace::buildShaderProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, TRACE_VERT);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, TRACE_FRAG);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint status = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (!status) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len);
        glGetProgramInfoLog(prog, len, nullptr, log.data());
        std::cerr << "CameraTrace program link error:\n" << log.data() << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// --------------------- GL setup ---------------------

void CameraTrace::buildGL() {
    if (m_vao != 0) return; // already built

    m_prog = buildShaderProgram();

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // We'll fill data dynamically; allocate 0 for now
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    // layout: vec3 position, float alpha => 4 floats total
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void CameraTrace::init() {
    buildGL();
}

void CameraTrace::destroy() {
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_prog) glDeleteProgram(m_prog);
    m_vbo = m_vao = m_prog = 0;
    m_nodes.clear();
}

void CameraTrace::reset() {
    m_nodes.clear();
}

// --------------------- Update logic ---------------------

void CameraTrace::update(float dt, const glm::vec3 &camPos) {
    // Add new node if we moved far enough from the last recorded position
    if (m_nodes.empty() ||
        glm::length2(camPos - m_nodes.back().pos) > m_minSegmentDist * m_minSegmentDist) {
        m_nodes.push_back({camPos, 0.0f});
    }

    // Age all nodes
    for (auto &n : m_nodes) {
        n.age += dt;
    }

    // Drop nodes that are too old (diminishing length)
    while (!m_nodes.empty() && m_nodes.front().age > m_maxAge) {
        m_nodes.erase(m_nodes.begin());
    }
}

// --------------------- Draw (Bezier-like fading curve) ---------------------

void CameraTrace::draw(const glm::mat4 &view, const glm::mat4 &proj) {
    if (!m_prog || m_nodes.size() < 3) return;

    // Build a piecewise quadratic Bezier approximation through the nodes.
    // For nodes P0,P1,P2,... we use each triple (Pi,Pi+1,Pi+2) as a quadratic Bezier.
    const int SAMPLES_PER_SEG = 8;

    std::vector<float> verts;
    verts.reserve((m_nodes.size() - 2) * (SAMPLES_PER_SEG + 1) * 4);

    for (size_t i = 0; i + 2 < m_nodes.size(); ++i) {
        const Node &n0 = m_nodes[i];
        const Node &n1 = m_nodes[i + 1];
        const Node &n2 = m_nodes[i + 2];

        for (int j = 0; j <= SAMPLES_PER_SEG; ++j) {
            float t = float(j) / float(SAMPLES_PER_SEG);
            float u = 1.0f - t;

            // Quadratic Bezier interpolation
            glm::vec3 p = u*u * n0.pos + 2.0f*u*t * n1.pos + t*t * n2.pos;

            // Age interpolation (approximate): same quadratic blend
            float age = u*u * n0.age + 2.0f*u*t * n1.age + t*t * n2.age;
            float alpha = 1.0f - glm::clamp(age / m_maxAge, 0.0f, 1.0f);

            verts.push_back(p.x);
            verts.push_back(p.y);
            verts.push_back(p.z);
            verts.push_back(alpha);
        }
    }

    if (verts.empty()) return;

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 verts.size() * sizeof(float),
                 verts.data(),
                 GL_DYNAMIC_DRAW);

    glUseProgram(m_prog);

    GLint locView = glGetUniformLocation(m_prog, "view");
    GLint locProj = glGetUniformLocation(m_prog, "proj");
    glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(proj));

    glBindVertexArray(m_vao);

    // enable blending so the alpha fade works nicely
    GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    if (!blendWasEnabled) glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Optional: make line a bit thicker (actual width is implementation-dependent)
    glLineWidth(2.0f);

    GLsizei count = static_cast<GLsizei>(verts.size() / 4);
    glDrawArrays(GL_LINE_STRIP, 0, count);

    if (!blendWasEnabled) glDisable(GL_BLEND);

    glBindVertexArray(0);
    glUseProgram(0);
}
