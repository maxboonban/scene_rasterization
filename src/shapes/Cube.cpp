#include "Cube.h"

void cubeInsertVec3(std::vector<GLfloat> &data, glm::vec3 v)
{
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}

void makeCubeTile(glm::vec3 topLeft,
                  glm::vec3 topRight,
                  glm::vec3 bottomLeft,
                  glm::vec3 bottomRight,
                  std::vector<GLfloat> &m_vertexData)
{
    glm::vec3 vec1, vec2, vecNormal;

    // top tri
    cubeInsertVec3(m_vertexData, topLeft);
    vec1 = bottomRight - topLeft;
    vec2 = topRight - topLeft;
    vecNormal = glm::cross(vec1, vec2);
    vecNormal = glm::normalize(vecNormal);
    cubeInsertVec3(m_vertexData, vecNormal);

    cubeInsertVec3(m_vertexData, bottomRight);
    vec2 = topRight - bottomRight;
    vecNormal = glm::cross(vec1, vec2);
    vecNormal = glm::normalize(vecNormal);
    cubeInsertVec3(m_vertexData, vecNormal);

    cubeInsertVec3(m_vertexData, topRight);
    vec1 = topLeft - topRight;
    vec2 = bottomRight - topRight;
    vecNormal = glm::cross(vec1, vec2);
    vecNormal = glm::normalize(vecNormal);
    cubeInsertVec3(m_vertexData, vecNormal);

    // bottom tri
    cubeInsertVec3(m_vertexData, topLeft);
    vec1 = bottomLeft - topLeft;
    vec2 = bottomRight - topLeft;
    vecNormal = glm::cross(vec1, vec2);
    vecNormal = glm::normalize(vecNormal);
    cubeInsertVec3(m_vertexData, vecNormal);

    cubeInsertVec3(m_vertexData, bottomLeft);
    vec2 = bottomRight - bottomLeft;
    vecNormal = glm::cross(vec1, vec2);
    vecNormal = glm::normalize(vecNormal);
    cubeInsertVec3(m_vertexData, vecNormal);

    cubeInsertVec3(m_vertexData, bottomRight);
    vec1 = topLeft - bottomRight;
    vec2 = bottomLeft - bottomRight;
    vecNormal = glm::cross(vec1, vec2);
    vecNormal = glm::normalize(vecNormal);
    cubeInsertVec3(m_vertexData, vecNormal);
}

void makeCubeFace(int m_param1, glm::vec3 topLeft,
                  glm::vec3 topRight,
                  glm::vec3 bottomLeft,
                  glm::vec3 bottomRight,
                  std::vector<GLfloat> &data)
{
    float tileLen = 1.f / m_param1;
    glm::vec3 top = topRight - topLeft;
    top = top * tileLen;
    glm::vec3 left = bottomLeft - topLeft;
    left = left * tileLen;
    for (int r = 0; r < m_param1; r++)
    {
        for (int c = 0; c < m_param1; c++)
        {
            makeCubeTile(
                topLeft + (r + 0.f) * left + (c + 0.f) * top,
                topLeft + (r + 0.f) * left + (c + 1.f) * top,
                topLeft + (r + 1.f) * left + (c + 0.f) * top,
                topLeft + (r + 1.f) * left + (c + 1.f) * top,
                data);
        }
    }
}

void Cube::makeCube(int m_param1, int m_param2, std::vector<GLfloat> &data)
{
    m_param1 = glm::max(1, m_param1);

    makeCubeFace(m_param1,
                 glm::vec3(-0.5f, 0.5f, 0.5f),
                 glm::vec3(0.5f, 0.5f, 0.5f),
                 glm::vec3(-0.5f, -0.5f, 0.5f),
                 glm::vec3(0.5f, -0.5f, 0.5f),
                 data);

    makeCubeFace(m_param1,
                 glm::vec3(0.5f, 0.5f, 0.5f),
                 glm::vec3(0.5f, 0.5f, -0.5f),
                 glm::vec3(0.5f, -0.5f, 0.5f),
                 glm::vec3(0.5f, -0.5f, -0.5f),
                 data);

    makeCubeFace(m_param1,
                 glm::vec3(0.5f, 0.5f, -0.5f),
                 glm::vec3(-0.5f, 0.5f, -0.5f),
                 glm::vec3(0.5f, -0.5f, -0.5f),
                 glm::vec3(-0.5f, -0.5f, -0.5f),
                 data);

    makeCubeFace(m_param1,
                 glm::vec3(-0.5f, 0.5f, -0.5f),
                 glm::vec3(-0.5f, 0.5f, 0.5f),
                 glm::vec3(-0.5f, -0.5f, -0.5f),
                 glm::vec3(-0.5f, -0.5f, 0.5f),
                 data);

    makeCubeFace(m_param1,
                 glm::vec3(-0.5f, 0.5f, -0.5f),
                 glm::vec3(0.5f, 0.5f, -0.5f),
                 glm::vec3(-0.5f, 0.5f, 0.5f),
                 glm::vec3(0.5f, 0.5f, 0.5f),
                 data);

    makeCubeFace(m_param1,
                 glm::vec3(-0.5f, -0.5f, 0.5f),
                 glm::vec3(0.5f, -0.5f, 0.5f),
                 glm::vec3(-0.5f, -0.5f, -0.5f),
                 glm::vec3(0.5f, -0.5f, -0.5f),
                 data);
}
