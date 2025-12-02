#include "Cylinder.h"

void Cylinder::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 = param1;
    m_param2 = param2;
    setVertexData();
}

void Cylinder::makeCapTile(glm::vec3 topLeft,
                           glm::vec3 topRight,
                           glm::vec3 bottomLeft,
                           glm::vec3 bottomRight) {
    glm::vec3 norm1 = glm::normalize(glm::cross(topLeft-bottomRight, bottomLeft-bottomRight));
    glm::vec3 norm2 = glm::normalize(glm::cross(topLeft-topRight, bottomRight-topRight));
    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, norm1);
    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, norm1);
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, norm1);
    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, norm2);
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, norm2);
    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, norm2);
}

glm::vec3 Cylinder::cylindricalToCartesian(float r, float y, float theta) {
    float x = r * glm::cos(theta);
    float z = r * glm::sin(theta);
    return glm::vec3(x, y, z);
}

void Cylinder::makeBottomCapSlice(float currentTheta, float nextTheta) {
    float rStep = 0.5f / m_param1;
    for (int i = 0; i < m_param1; i++) {
        float r = rStep * i;
        glm::vec3 TL = cylindricalToCartesian(r, -0.5f, currentTheta);
        glm::vec3 TR = cylindricalToCartesian(r, -0.5f, nextTheta);
        glm::vec3 BL = cylindricalToCartesian(r+rStep, -0.5f, currentTheta);
        glm::vec3 BR = cylindricalToCartesian(r+rStep, -0.5f, nextTheta);
        makeCapTile(TL, TR, BL, BR);
    }
}

void Cylinder::makeTopCapSlice(float currentTheta, float nextTheta) {
    float rStep = 0.5f / m_param1;
    for (int i = 0; i < m_param1; i++) {
        float r = rStep * i;
        glm::vec3 TL = cylindricalToCartesian(r, 0.5f, nextTheta);
        glm::vec3 TR = cylindricalToCartesian(r, 0.5f, currentTheta);
        glm::vec3 BL = cylindricalToCartesian(r+rStep, 0.5f, nextTheta);
        glm::vec3 BR = cylindricalToCartesian(r+rStep, 0.5f, currentTheta);
        makeCapTile(TL, TR, BL, BR);
    }
}

glm::vec3 Cylinder::calcNorm(glm::vec3& pt) {
    float xNorm = (2 * pt.x);
    float yNorm = 0.f;
    float zNorm = (2 * pt.z);

    return glm::normalize(glm::vec3{ xNorm, yNorm, zNorm });
}

void Cylinder::makeBodyTile(glm::vec3 topLeft,
                            glm::vec3 topRight,
                            glm::vec3 bottomLeft,
                            glm::vec3 bottomRight) {
    glm::vec3 normBL = calcNorm(bottomLeft);
    glm::vec3 normBR = calcNorm(bottomRight);
    glm::vec3 normTL = calcNorm(topLeft);
    glm::vec3 normTR = calcNorm(topRight);
    if (topLeft == topRight) {
        glm::vec3 bottomAvg = glm::normalize((normBL + normBR) / 2.f);
        normTL = bottomAvg;
        normTR = bottomAvg;
    }
    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normTL);
    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, normBL);
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normBR);
    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normTL);
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normBR);
    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, normTR);
}

void Cylinder::makeBodySlice(float currentTheta, float nextTheta){
    float yStep = 1.f / m_param1;
    float r = 0.5f;
    for (int i = 0; i < m_param1; i++) {
        float y = 0.5f - yStep * i;
        glm::vec3 TR = cylindricalToCartesian(r, y, currentTheta);
        glm::vec3 TL = cylindricalToCartesian(r, y, nextTheta);
        glm::vec3 BR = cylindricalToCartesian(r, y-yStep, currentTheta);
        glm::vec3 BL = cylindricalToCartesian(r, y-yStep, nextTheta);
        makeBodyTile(TL, TR, BL, BR);
    }
}

void Cylinder::makeWedge(float currentTheta, float nextTheta) {
    makeBodySlice(currentTheta, nextTheta);
    makeTopCapSlice(currentTheta, nextTheta);
    makeBottomCapSlice(currentTheta, nextTheta);
}

void Cylinder::setVertexData() {
    // TODO for Project 5: Lights, Camera
    float thetaStep = glm::radians(360.f / m_param2);
    for (int i = 0; i < m_param2; i++) {
        float theta = thetaStep * i;
        makeWedge(theta, theta + thetaStep);
    }
}

// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Cylinder::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
