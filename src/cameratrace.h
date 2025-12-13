#pragma once

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

class CameraTrace {
public:
    CameraTrace() = default;

    void init();
    void destroy();
    void reset();

    // dt in seconds, camPos in world space
    void update(float dt, const glm::vec3 &camPos);

    // Draws the trace in the currently-bound framebuffer
    void draw(const glm::mat4 &view, const glm::mat4 &proj);

private:
    struct Node {
        glm::vec3 pos;
        float age; // seconds since recorded
    };

    std::vector<Node> m_nodes;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_prog = 0;

    // trail tuning
    float m_maxAge = 3.0f;        // seconds before a node disappears
    float m_minSegmentDist = 0.05f; // world-space distance before adding new node

    void buildGL();
    GLuint buildShaderProgram();
};
