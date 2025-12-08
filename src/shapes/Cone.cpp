#include "Cone.h"

glm::vec3 coneCircumPoint(float theta, float yLevel)
{
    float x = 0.5 * glm::cos(theta);
    float z = 0.5 * glm::sin(theta);
    return glm::vec3(x, yLevel, z);
}

void coneInsertVec3(std::vector<GLfloat> &data, glm::vec3 v)
{
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}

void makeConeCapTri(glm::vec3 point1, glm::vec3 point2, glm::vec3 point3,
                    std::vector<GLfloat> &m_vertexData)
{
    glm::vec3 vec1, vec2, vecNormal;
    coneInsertVec3(m_vertexData, point1);
    vec1 = point2 - point1;
    vec2 = point3 - point1;
    vecNormal = glm::cross(vec1, vec2);
    vecNormal = glm::normalize(vecNormal);
    coneInsertVec3(m_vertexData, vecNormal);
    coneInsertVec3(m_vertexData, point2);
    vec2 = point3 - point2;
    vecNormal = glm::cross(vec1, vec2);
    vecNormal = glm::normalize(vecNormal);
    coneInsertVec3(m_vertexData, vecNormal);
    coneInsertVec3(m_vertexData, point3);
    vec1 = point1 - point3;
    vec2 = point2 - point3;
    vecNormal = glm::cross(vec1, vec2);
    vecNormal = glm::normalize(vecNormal);
    coneInsertVec3(m_vertexData, vecNormal);
}

void makeConeCapTile(int m_param1, glm::vec3 point1, glm::vec3 point2, glm::vec3 point3, std::vector<GLfloat> &data)
{
    float sideLen = 1.f / m_param1;
    glm::vec3 left = point3 - point1;
    left = left * sideLen;
    glm::vec3 right = point3 - point2;
    right = right * sideLen;
    for (int i = 0; i < m_param1 - 1; i++)
    {
        float cur = i, next = i + 1;
        makeConeCapTri(point1 + left * cur, point2 + right * cur, point1 + left * next, data);
        makeConeCapTri(point2 + right * cur, point2 + right * next, point1 + left * next, data);
    }
    float cap = m_param1 - 1;
    makeConeCapTri(point1 + left * cap, point2 + right * cap, point3, data);
}

void makeConeCapSlice(int m_param1, float currentTheta, float nextTheta, std::vector<GLfloat> &data)
{
    glm::vec3 point3 = glm::vec3(0.f, -0.5, 0.f);
    glm::vec3 point1 = coneCircumPoint(currentTheta, -0.5);
    glm::vec3 point2 = coneCircumPoint(nextTheta, -0.5);
    makeConeCapTile(m_param1, point1, point2, point3, data);
}

glm::vec3 slopeNorm(glm::vec3 &pt)
{
    float xNorm = (2 * pt.x);
    float yNorm = -(1.f / 4.f) * (2.f * pt.y - 1.f);
    float zNorm = (2 * pt.z);

    return glm::normalize(glm::vec3{xNorm, yNorm, zNorm});
}

void makeConeSlopeTri(glm::vec3 point1, glm::vec3 point2, glm::vec3 point3, bool cap,
                      std::vector<GLfloat> &m_vertexData)
{
    glm::vec3 vecNormal;
    coneInsertVec3(m_vertexData, point1);
    if (cap)
        vecNormal = glm::normalize(slopeNorm(point2) + slopeNorm(point3));
    else
        vecNormal = slopeNorm(point1);
    coneInsertVec3(m_vertexData, vecNormal);
    coneInsertVec3(m_vertexData, point2);
    vecNormal = slopeNorm(point2);
    coneInsertVec3(m_vertexData, vecNormal);
    coneInsertVec3(m_vertexData, point3);
    vecNormal = slopeNorm(point3);
    coneInsertVec3(m_vertexData, vecNormal);
}

void makeConeSlopeTile(int m_param1, glm::vec3 point1, glm::vec3 point2, glm::vec3 point3, std::vector<GLfloat> &data)
{
    float sideLen = 1.f / m_param1;
    glm::vec3 left = point3 - point1;
    left = left * sideLen;
    glm::vec3 right = point3 - point2;
    right = right * sideLen;
    for (int i = 0; i < m_param1 - 1; i++)
    {
        float cur = i, next = i + 1;
        makeConeSlopeTri(point1 + left * next, point1 + left * cur, point2 + right * next, false, data);
        makeConeSlopeTri(point1 + left * cur, point2 + right * cur, point2 + right * next, false, data);
    }
    float cap = m_param1 - 1;
    makeConeSlopeTri(point3, point1 + left * cap, point2 + right * cap, true, data);
}

void makeConeSlopeSlice(int m_param1, float currentTheta, float nextTheta, std::vector<GLfloat> &data)
{
    glm::vec3 point3 = glm::vec3(0.f, 0.5, 0.f);
    glm::vec3 point1 = coneCircumPoint(nextTheta, -0.5);
    glm::vec3 point2 = coneCircumPoint(currentTheta, -0.5);
    makeConeSlopeTile(m_param1, point1, point2, point3, data);
}

void makeConeWedge(int m_param1, float currentTheta, float nextTheta, std::vector<GLfloat> &data)
{
    makeConeCapSlice(m_param1, currentTheta, nextTheta, data);
    makeConeSlopeSlice(m_param1, currentTheta, nextTheta, data);
}

void Cone::makeCone(int m_param1, int m_param2, std::vector<GLfloat> &data)
{
    m_param1 = glm::max(1, m_param1);
    m_param2 = glm::max(3, m_param2);
    float PI = 3.141592;
    float arcLen = PI * 2 / m_param2;
    for (int i = 0; i < m_param2; i++)
    {
        float currentTheta = arcLen * i;
        float nextTheta = arcLen * (i + 1);
        makeConeWedge(m_param1, currentTheta, nextTheta, data);
    }
}