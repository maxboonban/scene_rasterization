#version 330 core

layout(location = 0) in vec3 posObj;

uniform mat4 model;
uniform mat4 lightMVP;

void main() {
        gl_Position = lightMVP * model * vec4(posObj, 1.0);
}
