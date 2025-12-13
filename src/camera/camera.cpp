#include "camera.h"
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

// ===============================================================
// Constructor
// ===============================================================

Camera::Camera(SceneCameraData cameraData) {
    pos = cameraData.pos;   pos.w = 1.f;
    look = cameraData.look; look.w = 0.f;
    up   = cameraData.up;   up.w   = 0.f;

    heightAngle = cameraData.heightAngle;
    aperture = cameraData.aperture;
    focalLength = cameraData.focalLength;

    look = glm::normalize(glm::vec4(glm::vec3(look), 0.f));
    up   = glm::normalize(glm::vec4(glm::vec3(up),   0.f));
}

// ===============================================================
// View Matrix
// ===============================================================

glm::mat4 Camera::getViewMatrix() const {
    glm::vec3 look3 = glm::vec3(look);
    glm::vec3 up3   = glm::vec3(up);
    glm::vec3 pos3  = glm::vec3(pos);

    glm::vec3 w = -glm::normalize(look3);
    glm::vec3 v = glm::normalize(up3 - glm::dot(up3, w) * w);
    glm::vec3 u = glm::cross(v, w);

    glm::mat4 R = glm::mat4(
        u.x,  v.x,  w.x,  0.f,
        u.y,  v.y,  w.y,  0.f,
        u.z,  v.z,  w.z,  0.f,
        0.f,  0.f,  0.f,  1.f
        );

    glm::mat4 T = glm::mat4(
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        -pos3.x, -pos3.y, -pos3.z, 1.f
        );

    return R * T;
}

// ===============================================================
// Projection Matrix
// ===============================================================

glm::mat4 Camera::getProjectionMatrix(float aspectRatio,
                                      float nearPlane,
                                      float farPlane) const
{
    float f = 1.f / glm::tan(heightAngle / 2.f);

    glm::mat4 proj(0.f);

    proj[0][0] = f / aspectRatio;
    proj[1][1] = f;

    proj[2][2] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    proj[2][3] = -1.f;

    proj[3][2] = -(2.f * farPlane * nearPlane) / (farPlane - nearPlane);

    return proj;
}

// ===============================================================
// Axis Helpers
// ===============================================================

glm::vec3 Camera::getW() const {
    return -glm::normalize(glm::vec3(look));
}

glm::vec3 Camera::getV() const {
    glm::vec3 w = getW();
    glm::vec3 up3 = glm::vec3(up);

    return glm::normalize(up3 - glm::dot(up3, w) * w);
}

glm::vec3 Camera::getU() const {
    return glm::cross(getV(), getW());
}

// ===============================================================
// MOVEMENT — applied to vec4 pos
// ===============================================================

void Camera::moveForward(float amt) {
    glm::vec3 newPos = glm::vec3(pos) + amt * glm::normalize(glm::vec3(look));
    pos = glm::vec4(newPos, 1.f);
}

void Camera::moveRight(float amt) {
    glm::vec3 right = glm::normalize(glm::cross(glm::vec3(look), glm::vec3(up)));
    glm::vec3 newPos = glm::vec3(pos) + amt * right;
    pos = glm::vec4(newPos, 1.f);
}

void Camera::moveUp(float amt) {
    glm::vec3 newPos = glm::vec3(pos) + amt * glm::vec3(0,1,0);
    pos = glm::vec4(newPos, 1.f);
}

// ===============================================================
// ROTATION — applied to vec4 look/up
// ===============================================================

static glm::vec3 rotateAroundAxis(const glm::vec3 &v,
                                  float theta,
                                  const glm::vec3 &k_normalized)
{
    float c = cos(theta);
    float s = sin(theta);
    const glm::vec3 &k = k_normalized;

    return v * c +
           glm::cross(k, v) * s +
           k * (glm::dot(k, v) * (1.0f - c));
}

void Camera::rotateYawPitch(float dYaw, float dPitch)
{
    glm::vec3 look3 = glm::vec3(look);
    glm::vec3 up3   = glm::vec3(up);

    // ---------- YAW (rotate around world up = +Y) ----------
    glm::vec3 worldUp(0.f, 1.f, 0.f);
    look3 = rotateAroundAxis(look3, dYaw, worldUp);
    up3   = rotateAroundAxis(up3,   dYaw, worldUp);

    // ---------- PITCH (rotate around camera right axis) ----------
    glm::vec3 right = glm::normalize(glm::cross(look3, up3));
    look3 = rotateAroundAxis(look3, dPitch, right);
    up3   = rotateAroundAxis(up3,   dPitch, right);

    // Normalize and write back
    look = glm::vec4(glm::normalize(look3), 0.f);
    up   = glm::vec4(glm::normalize(up3),   0.f);
}

void Camera::lookAtPoint(const glm::vec3 &target)
{
    glm::vec3 pos3 = glm::vec3(pos);

    // New forward direction
    glm::vec3 newLook = glm::normalize(target - pos3);

    // Old up
    glm::vec3 up3 = glm::vec3(up);

    // Compute right = look × up
    glm::vec3 right = glm::cross(newLook, up3);

    // Degenerate case: look is parallel to old up
    if (glm::length(right) < 1e-6f) {
        // Pick an arbitrary perpendicular vector
        if (fabs(newLook.y) > 0.9f)
            right = glm::cross(newLook, glm::vec3(1,0,0));
        else
            right = glm::cross(newLook, glm::vec3(0,1,0));
    }

    right = glm::normalize(right);

    // Recompute up = right × look
    glm::vec3 newUp = glm::normalize(glm::cross(right, newLook));

    // Write back
    look = glm::vec4(newLook, 0.f);
    up   = glm::vec4(newUp,   0.f);
}
