#include <stdexcept>
#include <glm/gtx/transform.hpp>
#include "camera.h"

glm::mat4 Camera::getProjectionMatrix(float near, float far) {
    // scaling matrix
    float heightAngle = getHeightAngle();
    float aspectRatio = getAspectRatio();
    float yscale = 1.f / (far * glm::tan(heightAngle/2.f));
    float xscale = yscale / aspectRatio;
    glm::mat4 scale = glm::mat4(xscale, 0.f,    0.f,     0.f,
                                0.f,    yscale, 0.f,     0.f,
                                0.f,    0.f,    1.f/far, 0.f,
                                0.f,    0.f,    0.f,     1.f);

    // unhinging matrix
    float c = -near / far;
    glm::mat4 unhinge = glm::mat4(1.f, 0.f,  0.f,          0.f,
                                  0.f, 1.f,  0.f,          0.f,
                                  0.f, 0.f,  1.f/(1.f+c), -1.f,
                                  0.f, 0.f, -c/(1.f+c),    0.f);

    // remap z to [-1, 1]
    glm::mat4 z = glm::mat4(1.f, 0.f,  0.f, 0.f,
                            0.f, 1.f,  0.f, 0.f,
                            0.f, 0.f, -2.f, 0.f,
                            0.f, 0.f, -1.f, 1.f);

    return z * unhinge * scale;
}

glm::mat4 Camera::getViewMatrix() const {
    glm::vec3 pos = glm::vec3(cameraData.pos);
    glm::vec3 look = glm::vec3(cameraData.look);
    glm::vec3 up = glm::vec3(cameraData.up);
    // Translation matrix
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), pos * -1.f);
    // Rotation matrix
    glm::vec3 w = glm::normalize(-look);
    glm::vec3 v = glm::normalize(up - glm::dot(up, w)*w);
    glm::vec3 u = glm::cross(v, w);
    glm::mat4 rotation = glm::mat4(u[0], v[0], w[0], 0.f,
                                   u[1], v[1], w[1], 0.f,
                                   u[2], v[2], w[2], 0.f,
                                   0.f, 0.f, 0.f, 1.f);
    return rotation * translation;
}

float Camera::getAspectRatio() const {
    return m_aspect;
}

float Camera::getHeightAngle() const {
    return cameraData.heightAngle;
}

float Camera::getFocalLength() const {
    throw std::runtime_error("not implemented");
}

float Camera::getAperture() const {
    throw std::runtime_error("not implemented");
}
