#include "transforms.h"
#include "glm/gtx/transform.hpp"
#include <math.h>

glm::mat4 Transforms::getRotationMatrix(float angle, glm::vec3 axis)
{
    glm::vec3 u = glm::normalize(axis);
    float angle_rad = glm::radians(angle);
    float cos = glm::cos(angle_rad);
    float sin = glm::sin(angle_rad);
    glm::vec4 col1 = glm::vec4(u.x * u.x * (1 - cos) + cos, u.x * u.y * (1 - cos) + u.z * sin, u.x * u.z * (1 - cos) - u.y * sin, 0);
    glm::vec4 col2 = glm::vec4(u.x * u.y * (1 - cos) - u.z * sin, u.y * u.y * (1 - cos) + cos, u.y * u.z * (1 - cos) + u.x * sin, 0);
    glm::vec4 col3 = glm::vec4(u.x * u.z * (1 - cos) + u.y * sin, u.y * u.z * (1 - cos) - u.x * sin, u.z * u.z * (1 - cos) + cos, 0);
    glm::vec4 trans = glm::vec4(0, 0, 0, 1);
    return glm::mat4(col1, col2, col3, trans);
}

glm::mat4 Transforms::getTranslationMatrix(float dx, float dy, float dz)
{
    glm::vec4 col1 = glm::vec4(1, 0, 0, 0);
    glm::vec4 col2 = glm::vec4(0, 1, 0, 0);
    glm::vec4 col3 = glm::vec4(0, 0, 1, 0);
    glm::vec4 trans = glm::vec4(dx, dy, dz, 1);
    return glm::mat4(col1, col2, col3, trans);
}
