#version 330 core

in vec3 posWorld;
in vec3 normWorld;
in vec4 posLightSpace[8];
out vec4 fragColor;

struct Light {
    int   type;       // 0 = point, 1 = directional, 2 = spot
    vec3  color;
    vec3  pos;
    vec3  dir;
    vec3  function;
    float angle;
    float penumbra;
};

uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float shininess;
uniform vec3 cameraPos;

uniform int numLights;
uniform Light lights[8];

uniform sampler2D shadowMaps[8];
uniform int shadowSize;
uniform bool softShadows;

// ============================================================
// computeShadow()  — NOW SAFE WHEN softShadows == false
// ============================================================
float computeShadow(int i, vec4 posLS, float bias)
{
    // If shadowing is OFF, skip ALL depth operations.
    if (!softShadows || shadowSize <= 0) {
        return 1.0;   // fully lit, NO SHADOW
    }

    // Perspective divide if spotlight
    if (lights[i].type == 2) {
        posLS /= posLS.w;
    }

    // Transform from [-1,1] to [0,1]
    vec3 projCoords = posLS.xyz * 0.5 + 0.5;

    // If outside shadow map region, fully lit.
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z > 1.0)
    {
        return 1.0;
    }

    float currentDepth = projCoords.z - bias;

    // ======================================================
    // HARD SHADOW (single sample)
    // ======================================================
    if (!softShadows) {
        float closest = texture(shadowMaps[i], projCoords.xy).r;
        return (closest < currentDepth) ? 0.0 : 1.0;
    }

    // ======================================================
    // SOFT SHADOW (PCF)
    // ======================================================

    float visibility = 0.0;
    int samples = 0;

    // simple 4×4 pcf
    for (float y = -1.5; y <= 1.5; y += 1.0) {
        for (float x = -1.5; x <= 1.5; x += 1.0) {
            vec2 offset = vec2(x, y) / shadowSize;
            float closestDepth = texture(shadowMaps[i], projCoords.xy + offset).r;
            visibility += (closestDepth >= currentDepth ? 1.0 : 0.0);
            samples++;
        }
    }
    return visibility / float(samples);
}

// ============================================================
// MAIN
// ============================================================
void main()
{
    vec3 N = normalize(normWorld);

    // ambient
    vec3 illumination = ka;

    for (int i = 0; i < numLights; i++) {
        Light Lgt = lights[i];

        // --------------------------------------------
        // Compute light vector and attenuation
        // --------------------------------------------
        vec3 L;
        float attenuation = 1.0;

        if (Lgt.type == 1) { // directional
            L = normalize(-Lgt.dir);
        }
        else { // point or spot
            L = Lgt.pos - posWorld;
            float dist = length(L);
            L = normalize(L);

            attenuation = 1.0 / (Lgt.function.x +
                                 dist * Lgt.function.y +
                                 dist * dist * Lgt.function.z);
            attenuation = clamp(attenuation, 0.0, 1.0);
        }

        // --------------------------------------------
        // Spot light cutoff
        // --------------------------------------------
        if (Lgt.type == 2) {
            vec3 spotDir = normalize(-Lgt.dir);
            float angle = acos(dot(spotDir, L));

            float outer = Lgt.angle;
            float inner = outer - Lgt.penumbra;

            if (angle > outer) {
                attenuation = 0.0;
            }
            else if (angle > inner) {
                float t = (angle - inner) / (outer - inner);
                float smo = -2.0 * t*t*t + 3.0 * t*t;
                attenuation *= (1.0 - smo);
            }
        }

        // --------------------------------------------
        // Compute bias
        // --------------------------------------------
        float NdotL = max(dot(N, L), 0.0);
        float bias = clamp(0.005 * tan(acos(NdotL)), 0.00001, 0.01);

        // --------------------------------------------
        // Shadow evaluation (NOW SAFE)
        // --------------------------------------------
        float visibility = computeShadow(i, posLightSpace[i], bias);

        // --------------------------------------------
        // Diffuse
        // --------------------------------------------
        vec3 diffuse = kd * NdotL;

        // --------------------------------------------
        // Specular
        // --------------------------------------------
        vec3 specular = vec3(0.0);
        if (NdotL > 0.0) {
            vec3 R = normalize(reflect(-L, N));
            vec3 E = normalize(cameraPos - posWorld);
            float RdotE = max(dot(R, E), 0.0);
            specular = ks * pow(RdotE, shininess);
        }

        illumination += visibility * attenuation * Lgt.color * (diffuse + specular);
    }

    fragColor = vec4(illumination, 1.0);
}
