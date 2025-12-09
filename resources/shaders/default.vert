#version 330 core

layout (location = 0) in vec3 posObj;
layout (location = 1) in vec3 nObj;
layout (location = 2) in vec2 uvObj;
out vec3 posWorld;
out vec3 normWorld;
out vec4 posLightSpace[8];
out vec2 texCoord;

uniform mat4 model;
uniform mat4 normalModel;
uniform mat4 view;
uniform mat4 proj;

uniform int numLights;
uniform mat4 lightMVPs[8];

void main() {
    // world space position
    vec4 posWorld4 = model * vec4(posObj, 1.0);
    posWorld = vec3(posWorld4);

    // world space normals
    normWorld = normalize(mat3(normalModel) * nObj);

    // uv coords
    texCoord = uvObj;

    // light space position for each light
    for (int i = 0; i < numLights; i++) {
        posLightSpace[i] = lightMVPs[i] * posWorld4;
    }

    // transform object space positions to clip space
    gl_Position = proj * view * posWorld4;
}
