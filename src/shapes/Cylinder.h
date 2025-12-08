#pragma once

#include <vector>
#include <glm/glm.hpp>

class Cylinder
{
public:
    void updateParams(int param1, int param2);
    std::vector<float> generateShape() { return m_vertexData; }

private:
    // Core shape builders
    void setVertexData();
    void makeWedge(float currentTheta, float nextTheta);
    void makeSideSlice(float currentTheta, float nextTheta);
    void makeCapSlice(float currentTheta, float nextTheta, bool isTop);
    void makeSideTile(glm::vec3 topLeft,
                      glm::vec3 topRight,
                      glm::vec3 bottomLeft,
                      glm::vec3 bottomRight);
    void makeCapTile(glm::vec3 topLeft,
                     glm::vec3 topRight,
                     glm::vec3 bottomLeft,
                     glm::vec3 bottomRight,
                     bool isTop);

    // Utility
    glm::vec3 calcNorm(glm::vec3& pt);
    void insertVec3(std::vector<float>& data, glm::vec3 v);

    // Members
    std::vector<float> m_vertexData;
    int m_param1;
    int m_param2;
    float m_radius = 0.5f;
};
