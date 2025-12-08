#include "Sphere.h"

void Sphere::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 = param1;
    m_param2 = param2;
    setVertexData();
}

void Sphere::makeTile(glm::vec3 topLeft,
                      glm::vec3 topRight,
                      glm::vec3 bottomLeft,
                      glm::vec3 bottomRight) {
    // Task 5: Implement the makeTile() function for a Sphere
    // Note: this function is very similar to the makeTile() function for Cube,
    //       but the normals are calculated in a different way!

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, glm::normalize(topLeft));

    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, glm::normalize(bottomLeft));

    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, glm::normalize(bottomRight));

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, glm::normalize(topLeft));

    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, glm::normalize(bottomRight));

    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, glm::normalize(topRight));
}

void Sphere::makeWedge(float currentTheta, float nextTheta) {
    // Task 6: create a single wedge of the sphere using the
    //         makeTile() function you implemented in Task 5
    // Note: think about how param 1 comes into play here!
    float radius = 0.5f;  // sphere radius
    float phiStep = glm::radians(180.f / m_param1);

    for (int i = 0; i < m_param1; ++i) {
        float phiTop = i * phiStep;
        float phiBottom = (i + 1) * phiStep;

        glm::vec3 topLeft(
            radius * glm::sin(phiTop) * glm::cos(currentTheta),
            radius * glm::cos(phiTop),
            -radius * glm::sin(phiTop) * glm::sin(currentTheta)
            );

        glm::vec3 topRight(
            radius * glm::sin(phiTop) * glm::cos(nextTheta),
            radius * glm::cos(phiTop),
            -radius * glm::sin(phiTop) * glm::sin(nextTheta)
            );

        glm::vec3 bottomLeft(
            radius * glm::sin(phiBottom) * glm::cos(currentTheta),
            radius * glm::cos(phiBottom),
            -radius * glm::sin(phiBottom) * glm::sin(currentTheta)
            );

        glm::vec3 bottomRight(
            radius * glm::sin(phiBottom) * glm::cos(nextTheta),
            radius * glm::cos(phiBottom),
            -radius * glm::sin(phiBottom) * glm::sin(nextTheta)
            );

        makeTile(topLeft, topRight, bottomLeft, bottomRight);
    }
}

void Sphere::makeSphere() {
    // Task 7: create a full sphere using the makeWedge() function you
    //         implemented in Task 6
    // Note: think about how param 2 comes into play here!
    float thetaStep = glm::radians(360.f / m_param2);

    for (int j = 0; j < m_param2; ++j) {
        float currentTheta = j * thetaStep;
        float nextTheta = (j + 1) * thetaStep;

        makeWedge(currentTheta, nextTheta);
    }
}

void Sphere::setVertexData() {
    // Uncomment these lines to make a wedge for Task 6, then comment them out for Task 7:

    // float thetaStep = glm::radians(360.f / m_param2);
    // float currentTheta = 0 * thetaStep;
    // float nextTheta = 1 * thetaStep;
    // makeWedge(currentTheta, nextTheta);

    // Uncomment these lines to make sphere for Task 7:

    makeSphere();
}

// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Sphere::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
