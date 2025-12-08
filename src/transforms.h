#pragma once

#include "glm/glm.hpp"

class Transforms
{
public:
    static glm::mat4 getRotationMatrix(float angle, glm::vec3 axis);

    static glm::mat4 getTranslationMatrix(float dx, float dy, float dz);
};
