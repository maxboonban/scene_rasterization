#include "Sphere.h"

void sphereInsertVec3(std::vector<GLfloat> &data, glm::vec3 v)
{
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}

void makeSphereTile(glm::vec3 topLeft,
                    glm::vec3 topRight,
                    glm::vec3 bottomLeft,
                    glm::vec3 bottomRight,
                    std::vector<GLfloat> &m_vertexData)
{
    glm::vec3 vec1, vec2, vecNormal;

    // top tri
    sphereInsertVec3(m_vertexData, topLeft);
    vecNormal = glm::normalize(topLeft);
    sphereInsertVec3(m_vertexData, vecNormal);

    sphereInsertVec3(m_vertexData, bottomRight);
    vecNormal = glm::normalize(bottomRight);
    sphereInsertVec3(m_vertexData, vecNormal);

    sphereInsertVec3(m_vertexData, topRight);
    vecNormal = glm::normalize(topRight);
    sphereInsertVec3(m_vertexData, vecNormal);

    // bottom tri
    sphereInsertVec3(m_vertexData, topLeft);
    vecNormal = glm::normalize(topLeft);
    sphereInsertVec3(m_vertexData, vecNormal);

    sphereInsertVec3(m_vertexData, bottomLeft);
    vecNormal = glm::normalize(bottomLeft);
    sphereInsertVec3(m_vertexData, vecNormal);

    sphereInsertVec3(m_vertexData, bottomRight);
    vecNormal = glm::normalize(bottomRight);
    sphereInsertVec3(m_vertexData, vecNormal);
}

glm::vec3 sphereCoords(float phi, float theta)
{
    float x = 0.5f * glm::sin(phi) * glm::cos(theta);
    float y = 0.5f * glm::cos(phi);
    float z = -0.5f * glm::sin(phi) * glm::sin(theta);
    return glm::vec3(x, y, z);
}

void makeSphereWedge(int m_param1, float currentTheta, float nextTheta, std::vector<GLfloat> &data)
{
    float PI = 3.141592;
    float arc_len = PI / m_param1;
    for (int i = 0; i < m_param1; i++)
    {
        makeSphereTile(sphereCoords(arc_len * i, currentTheta),
                       sphereCoords(arc_len * i, nextTheta),
                       sphereCoords(arc_len * (i + 1), currentTheta),
                       sphereCoords(arc_len * (i + 1), nextTheta),
                       data);
    }
}

void Sphere::makeSphere(int m_param1, int m_param2, std::vector<GLfloat> &data)
{
    float PI = 3.141592;
    m_param1 = glm::max(2, m_param1);
    m_param2 = glm::max(3, m_param2);
    float arc_len = 2 * PI / m_param2;
    for (int i = 0; i < m_param2; i++)
    {
        makeSphereWedge(m_param1, i * arc_len, (i + 1) * arc_len, data);
    }
}