#version 330 core

layout(location = 0) in vec3 objPos;
layout(location = 1) in vec3 objNormal;

out vec3 worldPos;
out vec3 worldNormal;

uniform mat4 modelMatInvT;
uniform mat4 modelMat;

uniform mat4 viewMat;
uniform mat4 projMat;

void main()
{
    vec4 objPos4 = vec4(objPos, 1.f);
    vec4 worldPos4 = modelMat * objPos4;
    worldPos = vec3(worldPos4);

    vec4 worldNormal4 = modelMatInvT * vec4(objNormal, 0.f);
    worldNormal = vec3(worldNormal4);

    vec4 screenPos = projMat * viewMat * worldPos4;
    gl_Position = vec4(screenPos.x, screenPos.y, -screenPos.z, screenPos.w);
}
