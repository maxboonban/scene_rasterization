#include "Cylinder.h"

glm::vec3 cylinderCircumPoint(float theta, float yLevel)
{
    float x = 0.5 * glm::cos(theta);
    float z = 0.5 * glm::sin(theta);
    return glm::vec3(x, yLevel, z);
}

void cylinderInsertVec3(std::vector<GLfloat> &data, glm::vec3 v)
{
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}

void makeCylinderCapTri(glm::vec3 point1, glm::vec3 point2, glm::vec3 point3, std::vector<GLfloat> &m_vertexData)
{
    glm::vec3 vec1, vec2, vecNormal;
    cylinderInsertVec3(m_vertexData, point1);
    vec1 = point2 - point1;
    vec2 = point3 - point1;
    vecNormal = glm::cross(vec1, vec2);
    vecNormal = glm::normalize(vecNormal);
    cylinderInsertVec3(m_vertexData, vecNormal);
    cylinderInsertVec3(m_vertexData, point2);
    vec2 = point3 - point2;
    vecNormal = glm::cross(vec1, vec2);
    vecNormal = glm::normalize(vecNormal);
    cylinderInsertVec3(m_vertexData, vecNormal);
    cylinderInsertVec3(m_vertexData, point3);
    vec1 = point1 - point3;
    vec2 = point2 - point3;
    vecNormal = glm::cross(vec1, vec2);
    vecNormal = glm::normalize(vecNormal);
    cylinderInsertVec3(m_vertexData, vecNormal);
}

void makeCylinderCapTile(int m_param1, glm::vec3 point1, glm::vec3 point2, glm::vec3 point3, std::vector<GLfloat> &data)
{
    float sideLen = 1.f / m_param1;
    glm::vec3 left = point3 - point1;
    left = left * sideLen;
    glm::vec3 right = point3 - point2;
    right = right * sideLen;
    for (int i = 0; i < m_param1 - 1; i++)
    {
        float cur = i, next = i + 1;
        makeCylinderCapTri(point1 + left * cur, point2 + right * cur, point1 + left * next, data);
        makeCylinderCapTri(point2 + right * cur, point2 + right * next, point1 + left * next, data);
    }
    float cap = m_param1 - 1;
    makeCylinderCapTri(point1 + left * cap, point2 + right * cap, point3, data);
}

void makeCylinderCapSlice(int m_param1, float currentTheta, float nextTheta, float yLevel, std::vector<GLfloat> &data)
{
    glm::vec3 point3 = glm::vec3(0.f, yLevel, 0.f);
    glm::vec3 point1 = cylinderCircumPoint(currentTheta, yLevel);
    glm::vec3 point2 = cylinderCircumPoint(nextTheta, yLevel);
    if (yLevel == -0.5)
        makeCylinderCapTile(m_param1, point1, point2, point3, data);
    else
        makeCylinderCapTile(m_param1, point2, point1, point3, data);
}

void makeCylinderSideTri(glm::vec3 topLeft,
                         glm::vec3 topRight,
                         glm::vec3 bottomLeft,
                         glm::vec3 bottomRight, std::vector<GLfloat> &m_vertexData)
{
    // Task 2: create a tile (i.e. 2 triangles) based on 4 given points.
    glm::vec3 vec1, vec2, vecNormal;
    cylinderInsertVec3(m_vertexData, topLeft);
    vec1 = bottomRight - topLeft;
    vec2 = topRight - topLeft;
    vecNormal = glm::cross(vec1, vec2);
    vecNormal = glm::normalize(vecNormal);
    cylinderInsertVec3(m_vertexData, vecNormal);
    cylinderInsertVec3(m_vertexData, bottomRight);
    vec2 = topRight - bottomRight;
    vecNormal = glm::cross(vec1, vec2);
    vecNormal = glm::normalize(vecNormal);
    cylinderInsertVec3(m_vertexData, vecNormal);
    cylinderInsertVec3(m_vertexData, topRight);
    vec1 = topLeft - topRight;
    vec2 = bottomRight - topRight;
    vecNormal = glm::cross(vec1, vec2);
    vecNormal = glm::normalize(vecNormal);
    cylinderInsertVec3(m_vertexData, vecNormal);

    cylinderInsertVec3(m_vertexData, topLeft);
    vec1 = bottomLeft - topLeft;
    vec2 = bottomRight - topLeft;
    vecNormal = glm::cross(vec1, vec2);
    vecNormal = glm::normalize(vecNormal);
    cylinderInsertVec3(m_vertexData, vecNormal);
    cylinderInsertVec3(m_vertexData, bottomLeft);
    vec2 = bottomRight - bottomLeft;
    vecNormal = glm::cross(vec1, vec2);
    vecNormal = glm::normalize(vecNormal);
    cylinderInsertVec3(m_vertexData, vecNormal);
    cylinderInsertVec3(m_vertexData, bottomRight);
    vec1 = topLeft - bottomRight;
    vec2 = bottomLeft - bottomRight;
    vecNormal = glm::cross(vec1, vec2);
    vecNormal = glm::normalize(vecNormal);
    cylinderInsertVec3(m_vertexData, vecNormal);
}

void makeCylinderSideTile(int m_param1, glm::vec3 topLeft,
                          glm::vec3 topRight,
                          glm::vec3 bottomLeft,
                          glm::vec3 bottomRight, std::vector<GLfloat> &data)
{
    // Task 3: create a single side of the cube out of the 4
    //         given points and makeCylinderTile()
    // Note: think about how param 1 affects the number of triangles on
    //       the face of the cube
    float tileLen = 1.f / m_param1;
    glm::vec3 top = topRight - topLeft;
    glm::vec3 left = bottomLeft - topLeft;
    left = left * tileLen;
    for (int r = 0; r < m_param1; r++)
    {
        makeCylinderSideTri(
            topLeft + (r + 0.f) * left,
            topLeft + (r + 0.f) * left + top,
            topLeft + (r + 1.f) * left,
            topLeft + (r + 1.f) * left + top, data);
    }
}

void makeCylinderSideSlice(int m_param1, float currentTheta, float nextTheta, std::vector<GLfloat> &data)
{
    glm::vec3 topLeft = cylinderCircumPoint(nextTheta, 0.5);
    glm::vec3 topRight = cylinderCircumPoint(currentTheta, 0.5);
    glm::vec3 bottomLeft = cylinderCircumPoint(nextTheta, -0.5);
    glm::vec3 bottomRight = cylinderCircumPoint(currentTheta, -0.5);
    makeCylinderSideTile(m_param1, topLeft, topRight, bottomLeft, bottomRight, data);
}

void makeCylinderWedge(int m_param1, float currentTheta, float nextTheta, std::vector<GLfloat> &data)
{
    makeCylinderCapSlice(m_param1, currentTheta, nextTheta, 0.5, data);
    makeCylinderCapSlice(m_param1, currentTheta, nextTheta, -0.5, data);
    makeCylinderSideSlice(m_param1, currentTheta, nextTheta, data);
}

void Cylinder::makeCylinder(int m_param1, int m_param2, std::vector<GLfloat> &data)
{
    m_param1 = glm::max(1, m_param1);
    m_param2 = glm::max(3, m_param2);
    float PI = 3.141592;
    float arcLen = PI * 2 / m_param2;
    for (int i = 0; i < m_param2; i++)
    {
        float currentTheta = arcLen * i;
        float nextTheta = arcLen * (i + 1);
        makeCylinderWedge(m_param1, currentTheta, nextTheta, data);
    }
}
