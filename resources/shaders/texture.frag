#version 330 core
in vec2 uv;
uniform sampler2D colorTexture;
uniform sampler2D depthTexture;

out vec4 fragColor;

void main()
{
    // fragColor = vec4(1);
    // fragColor = vec4(0.0, 1.0, 0.0, 1.0);
    fragColor = texture(colorTexture, uv);
    // fragColor = texture(depthTexture, uv);
    // float d = texture(depthTexture, uv).r;
    // fragColor = vec4(d, d, d, 1.0);
}
