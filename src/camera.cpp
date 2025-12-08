#include "camera.h"
#include <iostream>

void Camera::update(SceneCameraData cameraData, float nearPlane, float farPlane, float width, float height)
{
    worldPos = cameraData.pos;
    worldPos.w = 1.f;

    look = glm::vec3(cameraData.look);
    up = glm::vec3(cameraData.up);

    computeViewMatrix();

    near = nearPlane;
    far = farPlane;

    heightAngle = cameraData.heightAngle;
    aspectRatio = width / height;
}

void Camera::computeViewMatrix()
{
    glm::mat4 T = glm::mat4(1.f);
    T[3] = -worldPos;
    T[3][3] = 1;

    w = -look;
    w = glm::normalize(w);
    v = up - (glm::dot(up, w)) * w;
    v = glm::normalize(v);
    u = glm::cross(v, w);
    glm::mat3 R3 = glm::mat3(u, v, w);
    R3 = glm::transpose(R3);
    glm::mat4 R = glm::mat4(R3);

    viewMatrix = R * T;
}

glm::mat4 Camera::getViewMatrix() const
{
    // Optional TODO: implement the getter or make your own design
    return viewMatrix;
}

glm::mat4 Camera::getProjMatrix() const
{
    float c = -near / far;
    glm::mat4 A = glm::mat4(1, 0, 0, 0,
                            0, 1, 0, 0,
                            0, 0, 1 / (1 + c), -1,
                            0, 0, -c / (1 + c), 0);

    float xScale = 1 / (far * glm::tan(getWidthAngle() / 2));
    float yScale = 1 / (far * glm::tan(getHeightAngle() / 2));
    float zScale = 1 / far;
    glm::mat4 B = glm::mat4(xScale, 0, 0, 0,
                            0, yScale, 0, 0,
                            0, 0, zScale, 0,
                            0, 0, 0, 1);
    return A * B;
}

glm::vec4 Camera::getWorldPos() const
{
    return worldPos;
}
float Camera::getHeightAngle() const
{
    // Optional TODO: implement the getter or make your own design
    return heightAngle;
}

float Camera::getWidthAngle() const
{
    // Optional TODO: implement the getter or make your own design
    float halfTan = glm::tan(heightAngle / 2);
    return 2 * glm::atan(aspectRatio * halfTan);
}

void decomposeMat4(glm::mat4 &viewMat, glm::vec3 &T, glm::mat3 &R)
{
    T = glm::vec3(viewMat[3]);
    R = glm::mat3(viewMat);
}

glm::vec3 bezierVec3(glm::vec3 &T0, glm::vec3 &T1, glm::vec3 &T2, glm::vec3 &T3, float t)
{
    float u = 1 - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;
    return T0 * uuu + T1 * (3 * uu * t) + T2 * (3 * u * tt) + T3 * ttt;
}

glm::quat bezierQuat(glm::quat &q0, glm::quat &q1, glm::quat &q2, glm::quat &q3, float t)
{
    glm::quat q01 = glm::slerp(q0, q1, t);
    glm::quat q12 = glm::slerp(q1, q2, t);
    glm::quat q23 = glm::slerp(q2, q3, t);

    glm::quat q012 = glm::slerp(q01, q12, t);
    glm::quat q123 = glm::slerp(q12, q23, t);

    return glm::slerp(q012, q123, t);
}

glm::mat3 bezierMat3(glm::mat3 &R0, glm::mat3 &R1, glm::mat3 &R2, glm::mat3 &R3, float t)
{
    glm::quat q0 = glm::quat_cast(R0);
    glm::quat q1 = glm::quat_cast(R1);
    glm::quat q2 = glm::quat_cast(R2);
    glm::quat q3 = glm::quat_cast(R3);

    glm::quat q = bezierQuat(q0, q1, q2, q3, t);

    return glm::mat3_cast(q);
}

void Camera::cubicBezier(glm::mat4 &M0, glm::mat4 &M1, glm::mat4 &M2, glm::mat4 &M3, float t)
{
    glm::vec3 T0, T1, T2, T3;
    glm::mat3 R0, R1, R2, R3;

    decomposeMat4(M0, T0, R0);
    decomposeMat4(M1, T1, R1);
    decomposeMat4(M2, T2, R2);
    decomposeMat4(M3, T3, R3);

    glm::vec3 T = bezierVec3(T0, T1, T2, T3, t);
    glm::mat3 R = bezierMat3(R0, R1, R2, R3, t);

    viewMatrix = glm::mat4(R);
    viewMatrix[3] = glm::vec4(T, 1.f);
}