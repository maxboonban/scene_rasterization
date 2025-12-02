#include "realtime.h"
#include <iostream>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <cmath>

// Computes the rotation matrix about the given axis for the given angle in radians
glm::mat4 Realtime::rodrigues(const float theta, const glm::vec3 &axis) {
    // Normalize the axis
    glm::vec3 nAxis = glm::normalize(axis);
    float ux = nAxis[0];
    float uy = nAxis[1];
    float uz = nAxis[2];

    float cosT = std::cos(theta);
    float sinT = std::sin(theta);

    glm::mat4 rotation = glm::mat4(cosT+ux*ux*(1.f-cosT),    ux*uy*(1.f-cosT)+uz*sinT, ux*uz*(1.f-cosT)-uy*sinT, 0.f,
                                   ux*uy*(1.f-cosT)-uz*sinT, cosT+uy*uy*(1.f-cosT),    uy*uz*(1.f-cosT)+ux*sinT, 0.f,
                                   ux*uz*(1.f-cosT)+uy*sinT, uy*uz*(1.f-cosT)-ux*sinT, cosT+uz*uz*(1.f-cosT),    0.f,
                                   0.f,                      0.f,                      0.f,                      1.f);
    return rotation;
}

// Rotates the camera look and up
void Realtime::updateMouseRotation() {
    // left/right rotation
    glm::mat4 rot = rodrigues(glm::radians(-10 * m_angleX), glm::vec3(0.f, 1.f, 0.f));
    m_camLook = glm::normalize(glm::vec3(rot * glm::vec4(m_camLook, 0.f)));
    m_camUp = glm::normalize(glm::vec3(rot * glm::vec4(m_camUp, 0.f)));

    // up/down rotation
    rot = rodrigues(glm::radians(-10 * m_angleY), glm::cross(m_camLook, m_camUp));
    m_camLook = glm::normalize(glm::vec3(rot * glm::vec4(glm::normalize(m_camLook), 0.f)));
    m_camUp = glm::normalize(glm::vec3(rot * glm::vec4(m_camUp, 0.f)));

    update();
}
