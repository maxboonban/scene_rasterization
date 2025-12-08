#version 330 core
in vec2 uv;
uniform sampler2D colorTexture;
uniform sampler2D depthTexture;

out vec4 fragColor;

void main()
{
    fragColor = texture(colorTexture, uv);
}
