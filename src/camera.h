#ifndef CAMERA_H
#define CAMERA_H

#include "utils/scenedata.h"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

class Camera
{
public:
    void update(SceneCameraData cameraData, float nearPlane, float farPlane, float width, float height);
    void computeViewMatrix();

    glm::vec3 look, up;
    glm::vec3 u, v, w;

    glm::mat4 viewMatrix;
    glm::vec4 worldPos;

    float near, far;
    float aspectRatio, heightAngle;
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjMatrix() const;

    glm::vec4 getWorldPos() const;
    float getHeightAngle() const;
    float getWidthAngle() const;

    void cubicBezier(glm::mat4 &M0, glm::mat4 &M1, glm::mat4 &M2, glm::mat4 &M3, float t);
};

#endif // CAMERA_H
