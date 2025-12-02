#include "Cone.h"
#include "iostream"

void Cone::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 = param1;
    m_param2 = param2;
    setVertexData();
}

// Task 8: create function(s) to make tiles which you can call later on
// Note: Consider your makeTile() functions from Sphere and Cube

void Cone::makeCapTile(glm::vec3 topLeft,
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

glm::vec3 Cone::cylindricalToCartesian(float r, float y, float theta) {
    float x = r * glm::cos(theta);
    float z = r * glm::sin(theta);
    return glm::vec3(x, y, z);
}

void Cone::makeCapSlice(float currentTheta, float nextTheta){
    // Task 8: create a slice of the cap face using your
    //         make tile function(s)
    // Note: think about how param 1 comes into play here!
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

glm::vec3 Cone::calcNorm(glm::vec3& pt) {
    float xNorm = (2 * pt.x);
    float yNorm = -(1.f/4.f) * (2.f * pt.y - 1.f);
    float zNorm = (2 * pt.z);

    return glm::normalize(glm::vec3{ xNorm, yNorm, zNorm });
}

void Cone::makeSlopeTile(glm::vec3 topLeft,
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

void Cone::makeSlopeSlice(float currentTheta, float nextTheta){
    // Task 9: create a single sloped face using your make
    //         tile function(s)
    // Note: think about how param 1 comes into play here!
    float yStep = 1.f / m_param1;
    float rStep = 0.5f / m_param1;
    for (int i = 0; i < m_param1; i++) {
        float y = 0.5f - yStep * i;
        float r = rStep * i;
        glm::vec3 TR = cylindricalToCartesian(r, y, currentTheta);
        glm::vec3 TL = cylindricalToCartesian(r, y, nextTheta);
        glm::vec3 BR = cylindricalToCartesian(r+rStep, y-yStep, currentTheta);
        glm::vec3 BL = cylindricalToCartesian(r+rStep, y-yStep, nextTheta);
        makeSlopeTile(TL, TR, BL, BR);
    }
}

void Cone::makeWedge(float currentTheta, float nextTheta) {
    // Task 10: create a single wedge of the Cone using the
    //          makeCapSlice() and makeSlopeSlice() functions you
    //          implemented in Task 5
    makeSlopeSlice(currentTheta, nextTheta);
    makeCapSlice(currentTheta, nextTheta);
}

void Cone::setVertexData() {
    // Task 10: create a full cone using the makeWedge() function you
    //          just implemented
    // Note: think about how param 2 comes into play here!
    float thetaStep = glm::radians(360.f / m_param2);
    for (int i = 0; i < m_param2; i++) {
        float theta = thetaStep * i;
        makeWedge(theta, theta + thetaStep);
    }
}

// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Cone::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
