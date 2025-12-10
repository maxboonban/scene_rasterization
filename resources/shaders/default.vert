#version 330 core

layout(location = 0) in vec3 posObj;
layout(location = 1) in vec3 nObj;
layout(location = 2) in vec2 uv;

uniform mat4 model;
uniform mat4 normalModel;
uniform mat4 view;
uniform mat4 proj;

uniform int numLights;
uniform mat4 lightMVPs[8];

out vec3 posWorld;          // your fragment shader expects this
out vec3 normWorld;         // your fragment shader expects this
out vec4 posLightSpace[8];  // required for soft shadow code
out vec2 uvOut;

void main()
{
    // World-space position
    vec4 pw = model * vec4(posObj, 1.0);
    posWorld = pw.xyz;

    // World-space normal
    normWorld = normalize(mat3(normalModel) * nObj);

    // Shadow mapping coordinates
    for (int i = 0; i < numLights; i++) {
        posLightSpace[i] = lightMVPs[i] * pw;
    }

    gl_Position = proj * view * pw;
    uvOut = uv;
}
