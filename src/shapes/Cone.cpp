#include "Cone.h"

void Cone::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 = param1;
    m_param2 = param2;
    setVertexData();
}

// Task 8: create function(s) to make tiles which you can call later on
// Note: Consider your makeTile() functions from Sphere and Cube

glm::vec3 Cone::calcNorm(glm::vec3& pt) {
    float xNorm = (2 * pt.x);
    float yNorm = -(1.f/4.f) * (2.f * pt.y - 1.f);
    float zNorm = (2 * pt.z);

    return glm::normalize(glm::vec3{ xNorm, yNorm, zNorm });
}

void Cone::makeCapTile(glm::vec3 topLeft,
                       glm::vec3 topRight,
                       glm::vec3 bottomLeft,
                       glm::vec3 bottomRight) {
    glm::vec3 normal(0.f, -1.f, 0.f);

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

void Cone::makeSlopeTile(glm::vec3 topLeft,
                         glm::vec3 topRight,
                         glm::vec3 bottomLeft,
                         glm::vec3 bottomRight)
{
    // Put all vertices in an array so we don’t care how the function is called.
    glm::vec3 v[4] = { topLeft, topRight, bottomLeft, bottomRight };
    glm::vec3 n[4];

    const float eps = 1e-6f;

    // Find the two base vertices (non-tip, r > 0)
    int baseIdx[2];
    int baseCount = 0;
    for (int i = 0; i < 4; ++i) {
        float r2 = v[i].x * v[i].x + v[i].z * v[i].z;
        if (r2 > eps && baseCount < 2) {
            baseIdx[baseCount++] = i;
        }
    }

    // If we’re in the top slice (exactly 2 base vertices),
    // build a special tip normal from their normals.
    if (baseCount == 2) {
        glm::vec3 baseN0 = calcNorm(v[baseIdx[0]]);
        glm::vec3 baseN1 = calcNorm(v[baseIdx[1]]);
        glm::vec3 tipNormal = glm::normalize(baseN0 + baseN1);

        for (int i = 0; i < 4; ++i) {
            float r2 = v[i].x * v[i].x + v[i].z * v[i].z;
            if (r2 <= eps) {
                // tip vertex
                n[i] = tipNormal;
            } else {
                // base vertex
                n[i] = calcNorm(v[i]);
            }
        }
    } else {
        // Regular slope tile: all vertices get the implicit cone normal
        for (int i = 0; i < 4; ++i) {
            n[i] = calcNorm(v[i]);
        }
    }

    // Emit triangles using your original winding
    // (your makeSlopeSlice calls are inverted on purpose).
    insertVec3(m_vertexData, v[0]);
    insertVec3(m_vertexData, n[0]);
    insertVec3(m_vertexData, v[2]);
    insertVec3(m_vertexData, n[2]);
    insertVec3(m_vertexData, v[3]);
    insertVec3(m_vertexData, n[3]);

    insertVec3(m_vertexData, v[0]);
    insertVec3(m_vertexData, n[0]);
    insertVec3(m_vertexData, v[3]);
    insertVec3(m_vertexData, n[3]);
    insertVec3(m_vertexData, v[1]);
    insertVec3(m_vertexData, n[1]);
}

void Cone::makeCapSlice(float currentTheta, float nextTheta){
    // Task 8: create a slice of the cap face using your
    //         make tile function(s)
    // Note: think about how param 1 comes into play here!
    float radius = 0.5f;
    float y = -0.5f; // base of the cone

    // Step size for radius subdivision
    float rStep = radius / (float)m_param1;

    for (int i = 0; i < m_param1; i++) {
        float r1 = i * rStep;
        float r2 = (i + 1) * rStep;

        // Compute four corners of the current tile
        glm::vec3 topLeft(r1 * glm::cos(currentTheta), y, r1 * glm::sin(currentTheta));
        glm::vec3 topRight(r1 * glm::cos(nextTheta),   y, r1 * glm::sin(nextTheta));
        glm::vec3 bottomLeft(r2 * glm::cos(currentTheta), y, r2 * glm::sin(currentTheta));
        glm::vec3 bottomRight(r2 * glm::cos(nextTheta),   y, r2 * glm::sin(nextTheta));

        // Add this ring segment
        makeCapTile(topLeft, topRight, bottomLeft, bottomRight);
    }
}

void Cone::makeSlopeSlice(float currentTheta, float nextTheta){
    // Task 9: create a single sloped face using your make
    //         tile function(s)
    // Note: think about how param 1 comes into play here!
    float height = 1.0f;
    float yBase = -0.5f;
    float yStep = height / (float)m_param1;
    float rStep = 0.5f / (float)m_param1;

    for (int i = 0; i < m_param1; i++) {
        float rBottom = 0.5f - i * rStep;
        float rTop = 0.5f - (i + 1) * rStep;

        float yBottomCurr = yBase + i * yStep;
        float yTopCurr = yBase + (i + 1) * yStep;

        glm::vec3 bottomLeft(rBottom * glm::cos(currentTheta), yBottomCurr, rBottom * glm::sin(currentTheta));
        glm::vec3 bottomRight(rBottom * glm::cos(nextTheta),   yBottomCurr, rBottom * glm::sin(nextTheta));
        glm::vec3 topLeft(rTop * glm::cos(currentTheta), yTopCurr, rTop * glm::sin(currentTheta));
        glm::vec3 topRight(rTop * glm::cos(nextTheta),   yTopCurr, rTop * glm::sin(nextTheta));

        makeSlopeTile(bottomLeft, bottomRight, topLeft, topRight);
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
    float thetaStep = glm::radians(360.f / (float)m_param2);

    for (int i = 0; i < m_param2; ++i) {
        float currentTheta = i * thetaStep;
        float nextTheta = (i + 1) * thetaStep;

        makeWedge(currentTheta, nextTheta);
    }
}

// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Cone::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
