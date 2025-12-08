#include "Cylinder.h"
#include <glm/gtc/constants.hpp>
#include <cmath>

void Cylinder::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 = param1;
    m_param2 = param2;
    setVertexData();
}

glm::vec3 Cylinder::calcNorm(glm::vec3& pt) {
    return glm::normalize(glm::vec3(pt.x, 0.f, pt.z));
}

void Cylinder::makeCapTile(glm::vec3 topLeft,
                           glm::vec3 topRight,
                           glm::vec3 bottomLeft,
                           glm::vec3 bottomRight,
                           bool isTop) {
    glm::vec3 normal = isTop ? glm::vec3(0.f, 1.f, 0.f) : glm::vec3(0.f, -1.f, 0.f);

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normal);

    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, normal);

    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normal);

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normal);

    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normal);

    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, normal);
}

void Cylinder::makeSideTile(glm::vec3 topLeft,
                            glm::vec3 topRight,
                            glm::vec3 bottomLeft,
                            glm::vec3 bottomRight) {
    glm::vec3 normalTL = calcNorm(topLeft);
    glm::vec3 normalTR = calcNorm(topRight);
    glm::vec3 normalBL = calcNorm(bottomLeft);
    glm::vec3 normalBR = calcNorm(bottomRight);

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normalTL);

    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, normalBL);

    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normalBR);

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normalTL);

    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normalBR);

    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, normalTR);
}

void Cylinder::makeCapSlice(float currentTheta, float nextTheta, bool isTop) {
    float radius = 0.5f;
    float y = isTop ? 0.5f : -0.5f;
    float rStep = radius / (float)m_param1;

    for (int i = 0; i < m_param1; i++) {
        float r1 = i * rStep;
        float r2 = (i + 1) * rStep;

        glm::vec3 topLeft(r1 * glm::cos(currentTheta), y, r1 * glm::sin(currentTheta));
        glm::vec3 topRight(r1 * glm::cos(nextTheta),   y, r1 * glm::sin(nextTheta));
        glm::vec3 bottomLeft(r2 * glm::cos(currentTheta), y, r2 * glm::sin(currentTheta));
        glm::vec3 bottomRight(r2 * glm::cos(nextTheta),   y, r2 * glm::sin(nextTheta));

        if (isTop)
            makeCapTile(bottomLeft, bottomRight, topLeft, topRight, true);
        else
            makeCapTile(topLeft, topRight, bottomLeft, bottomRight, false);
    }
}

void Cylinder::makeSideSlice(float currentTheta, float nextTheta) {
    float height = 1.0f;
    float yBase = -0.5f;
    float yStep = height / (float)m_param1;

    for (int i = 0; i < m_param1; i++) {
        float yBottom = yBase + i * yStep;
        float yTop = yBase + (i + 1) * yStep;

        glm::vec3 bottomLeft(0.5f * glm::cos(currentTheta), yBottom, 0.5f * glm::sin(currentTheta));
        glm::vec3 bottomRight(0.5f * glm::cos(nextTheta),   yBottom, 0.5f * glm::sin(nextTheta));
        glm::vec3 topLeft(0.5f * glm::cos(currentTheta), yTop, 0.5f * glm::sin(currentTheta));
        glm::vec3 topRight(0.5f * glm::cos(nextTheta),   yTop, 0.5f * glm::sin(nextTheta));

        makeSideTile(bottomLeft, bottomRight, topLeft, topRight);
    }
}

void Cylinder::makeWedge(float currentTheta, float nextTheta) {
    makeSideSlice(currentTheta, nextTheta);
    makeCapSlice(currentTheta, nextTheta, true);   // top
    makeCapSlice(currentTheta, nextTheta, false);  // bottom
}

void Cylinder::setVertexData() {
    float thetaStep = glm::radians(360.f / (float)m_param2);

    for (int i = 0; i < m_param2; ++i) {
        float currentTheta = i * thetaStep;
        float nextTheta = (i + 1) * thetaStep;

        makeWedge(currentTheta, nextTheta);
    }
}

void Cylinder::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
