#version 330 core

in vec3 worldPos;
in vec3 worldNormal;

out vec4 fragColor;

uniform float ka;
uniform float kd;
uniform float ks;

uniform vec4 ca;
uniform vec4 cd;
uniform vec4 cs;
uniform float n;

uniform vec4 cameraPos;
uniform vec4 lightPos[8];
uniform vec4 lightDir[8];
uniform vec4 lightColor[8];
uniform vec3 lightFunc[8];
uniform float lightAngle[8];
uniform float lightPenumbra[8];
uniform int lightType[8];
uniform int numLights;

float getLightFalloff(vec3 light, vec3 dir, vec3 world, float angle, float penumbra)
{
    vec3 toPos = world - vec3(light);
    toPos = normalize(toPos);
    vec3 lightDir = vec3(dir);
    lightDir = normalize(lightDir);

    float theta = acos(dot(toPos, lightDir));
    float inner = angle - penumbra;
    float outer = angle;

    if (theta < inner)
        return 1.f;
    if (theta > outer)
        return 0.f;
    float det = (theta - inner) / (outer - inner);
    return 1.f + 2.f * det * det * det - 3.f * det * det;
}

float getLightAttenuation(vec3 light, vec3 func, vec3 world)
{
    float c1 = func.x;
    float c2 = func.y;
    float c3 = func.z;

    vec3 toPosition = vec3(light) - world;
    float distance = sqrt(dot(toPosition, toPosition));

    return min(1.f, 1.f / (c1 + distance * c2 + distance * distance * c3));
}

void main()
{
    fragColor = vec4(0.f);
    for (int i = 0; i < numLights; i += 1)
    {
        vec3 worldNormal = normalize(worldNormal);
        vec3 lightPos3 = vec3(lightPos[i]);
        vec3 lightDir3;
        if (lightType[i] == 0)
            lightDir3 = normalize(worldPos - lightPos3);
        else
            lightDir3 = normalize(vec3(lightDir[i]));

        float lightMod = 1;
        if (lightType[i] != 1)
            lightMod *= getLightAttenuation(lightPos3, lightFunc[i], worldPos);
        if (lightType[i] == 2)
            lightMod *= getLightFalloff(lightPos3, lightDir3, worldPos, lightAngle[i], lightPenumbra[i]);

        fragColor += ka * ca;

        vec4 diffuse = lightMod * kd * cd * lightColor[i] * clamp(dot(worldNormal, -lightDir3), 0, 1);
        fragColor += diffuse;

        vec3 reflected = lightDir3 - 2 * dot(lightDir3, worldNormal) * worldNormal;
        vec3 cameraPos3 = vec3(cameraPos);
        vec3 toCamera = normalize(worldPos - cameraPos3);
        float specular = dot(reflected, toCamera);
        if (specular > 0 && n > 0)
        {
            vec4 shine = lightMod * pow(specular, n) * ks * cs * lightColor[i];
            fragColor += shine;
        }
    }
}
