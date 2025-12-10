#version 330 core
in vec2 uv;
uniform sampler2D sceneTexture;

out vec4 fragColor;

void main() {
    vec3 color = texture(sceneTexture, uv).rgb;
    
    // Calculate brightness (luminance)
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    
    // Only output colors above threshold (adjust 0.7 to control bloom intensity)
    if (brightness > 0.7) {
        fragColor = vec4(color, 1.0);
    } else {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
