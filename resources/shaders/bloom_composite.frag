#version 330 core
in vec2 uv;
uniform sampler2D sceneTexture;
uniform sampler2D bloomTexture;

out vec4 fragColor;

void main() {
    vec3 scene = texture(sceneTexture, uv).rgb;
    vec3 bloom = texture(bloomTexture, uv).rgb;
    
    // Additive blending - add bloom to scene
    vec3 result = scene + bloom;
    
    fragColor = vec4(result, 1.0);
}