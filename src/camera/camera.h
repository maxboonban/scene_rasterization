#pragma once

#include <glm/glm.hpp>
#include "utils/scenedata.h"

class Camera
{
public:
    // Construct from SceneCameraData
    Camera(SceneCameraData cameraData = SceneCameraData());

    // ----------- MATRICES -----------
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio,
                                  float nearPlane,
                                  float farPlane) const;

    // ----------- CAMERA AXIS GETTERS -----------
    glm::vec3 getU() const;  // Right
    glm::vec3 getV() const;  // True Up
    glm::vec3 getW() const;  // Backward

    // ----------- BASIC GETTERS -----------
    glm::vec4 getPosition() const { return pos; }
    float getHeightAngle() const  { return heightAngle; }
    float getFocalLength() const  { return focalLength; }
    float getAperture() const     { return aperture; }

    // ----------- MOVEMENT -----------
    void moveForward(float amt);
    void moveRight(float amt);
    void moveUp(float amt);

    // ----------- ROTATION -----------
    // yaw = rotation around world-space Y axis
    // pitch = rotation around camera right axis
    void rotateYawPitch(float dYaw, float dPitch);
    void lookAtPoint(const glm::vec3 &target);

public:
    // You access these directly in Realtime::paintGL
    glm::vec4 pos;
    glm::vec4 look;
    glm::vec4 up;

private:
    float heightAngle;  // vertical FOV (radians)
    float aperture;
    float focalLength;
};
