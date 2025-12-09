#pragma once

#include <vector>
#include <glm/glm.hpp>

class Cylinder
{
public:
    void updateParams(int param1, int param2);
    std::vector<float> generateShape() { return m_vertexData; }

private:
    void insertVec3(std::vector<float> &data, glm::vec3 v);
    glm::vec3 cylindricalToCartesian(float r, float y, float theta);

    // caps
    void makeCapTile(glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomLeft, glm::vec3 bottomRight);
    void makeBottomCapSlice(float currentTheta, float nextTheta);
    void makeTopCapSlice(float currentTheta, float nextTheta);

    // body
    glm::vec3 calcNorm(glm::vec3& pt);
    void makeBodyTile(glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomLeft, glm::vec3 bottomRight);
    void makeBodySlice(float currentTheta, float nextTheta);

    void makeWedge(float currentTheta, float nextTheta);
    void setVertexData();

    std::vector<float> m_vertexData;
    int m_param1;
    int m_param2;
    float m_radius = 0.5;
};
