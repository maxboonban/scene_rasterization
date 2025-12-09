#version 330 core

in vec3 posWorld;
in vec3 normWorld;
in vec4 posLightSpace[8];
in vec2 texCoord;

out vec4 fragColor;

struct Light {
    int   type;       // 0 = point, 1 = directional, 2 = spot
    vec3  color;
    vec3  pos;        // position of the light in world space (Not applicable to directional lights)
    vec3  dir;        // direction with CTM applied (Not applicable to point lights)
    vec3  function;   // attenuation function
    float angle;      // outer cone angle in radians (spot light only)
    float penumbra;   // angle between outer and inner cones (spot light only)
};

uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float shininess;
uniform vec3 cameraPos;

uniform int numLights;
uniform Light lights[8];   // supports up to 8 lights

// shadow mapping
uniform sampler2D shadowMaps[8];
uniform int shadowSize;
uniform bool softShadows;

// bump mapping
uniform sampler2D heightMap;
uniform float bumpScale;

// compute TBN
mat3 computeTBN(vec3 pos, vec2 uv, vec3 Ngeom) {
    vec3 dp1 = dFdx(pos);
    vec3 dp2 = dFdy(pos);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);

    float det = duv1.x * duv2.y - duv1.y * duv2.x;

    // if determinant is approx. 0
    if (abs(det) < 1e-6) {
        vec3 N = normalize(Ngeom);
        vec3 up = (abs(N.y) < 0.999) ? vec3(0,1,0) : vec3(1,0,0);
        vec3 T  = normalize(cross(up, N));
        vec3 B  = cross(N, T);
        return mat3(T, B, N);
    }

    float r = 1.0 / det;
    vec3 T = normalize((dp1 * duv2.y - dp2 * duv1.y) * r);
    vec3 B = normalize((dp2 * duv1.x - dp1 * duv2.x) * r);
    vec3 N = normalize(Ngeom);
    return mat3(T, B, N);
}

// perturbed normal
vec3 getBumpedNormal() {
    // compute TBN
    mat3 TBN = computeTBN(posWorld, texCoord, normWorld);

    // texture space pixel size
    ivec2 texSize = textureSize(heightMap, 0);
    vec2 texel = 1.0 / vec2(texSize);

    // sample heights
    float h  = texture(heightMap, texCoord).r;
    float hx = texture(heightMap, texCoord + vec2(texel.x, 0.0)).r;
    float hy = texture(heightMap, texCoord + vec2(0.0, texel.y)).r;

    // finite differences
    float Fu = hx - h;
    float Fv = hy - h;

    // perturbed normal
    vec3 N_tangent = normalize(vec3(-Fu * bumpScale, -Fv * bumpScale, 1.0));

    // transform back to world space
    return normalize(TBN * N_tangent);
}

void main() {
   // normalize normals
   // vec3 N = normalize(normWorld);
   vec3 N = getBumpedNormal();

   // ambient
   vec3 illumination = ka;

   // direction to light source
   vec3 L;

   // iterate through lights
   for (int i = 0; i < numLights; i++) {
      Light light = lights[i];

      // attenuation
      float attenuation = 1.0f;

      if (light.type == 1) {                 // directional lights
         L = normalize(-light.dir);
      } else {                               // point and spot lights
         L = light.pos - posWorld;
         float distance = length(L);
         attenuation = min(1.0f, 1.0f / (light.function[0] + distance*light.function[1] + distance*distance*light.function[2]));
         L = normalize(L);                   // normalize L
      }

      // spot light fallout
      if (light.type == 2) {
         vec3 spotlightDir = normalize(-light.dir);
         float pointAngle = acos(dot(spotlightDir, L));
         float outerTheta = light.angle;
         float innerTheta = outerTheta - light.penumbra;
         if (pointAngle > innerTheta && pointAngle <= outerTheta) {
            float t = (pointAngle - innerTheta) / (outerTheta - innerTheta);
            float falloff = -2.0f * t*t*t + 3.0f * t*t;
            attenuation *= (1.0f - falloff);
         } else if (pointAngle > outerTheta) {
            attenuation = 0.0f;
         }
      }

      // shadows
      // bias matrix to transform coordinates from [-1,1] to [0,1]
      mat4 biasMatrix = mat4(
                  0.5f, 0.0f, 0.0f, 0.0f,
                  0.0f, 0.5f, 0.0f, 0.0f,
                  0.0f, 0.0f, 0.5f, 0.0f,
                  0.5f, 0.5f, 0.5f, 1.0f);
      float NdotL = clamp(dot(N, L), 0.0f, 1.0f);
      // bias to avoid shadow acne/self-shadowing
      float bias = clamp(0.005*tan(acos(NdotL)), 0.00001f,0.01f);
      // light space position
      vec4 shadowCoord = posLightSpace[i];
      if (light.type == 2) { // spot light
         shadowCoord /= shadowCoord.w;
      }
      shadowCoord = biasMatrix * shadowCoord;
      // check whether to implement soft shadows
      float visibility;
      if (softShadows) {
         // PCF
         visibility = 0.0f;
         for (float y = -1.5f; y <= 1.5f; y+=1.0f) {
            for (float x = -1.5f; x <= 1.5f; x+=1.0f) {
               float closestDepth = texture(shadowMaps[i], shadowCoord.xy + vec2(x, y)/shadowSize).r;
               visibility += shadowCoord.z - bias > closestDepth ? 0.0f : 1.0f;
            }
         }
         visibility /= 16.0f;
      } else {
         // check if the closest depth is less than the current depth
         visibility = 1.0f;
         if (texture(shadowMaps[i], shadowCoord.xy).r  <  shadowCoord.z - bias){
            visibility = 0.0f;
         }
      }

      // diffuse
      vec3 diffuse = kd * NdotL;
      illumination += visibility * attenuation * light.color * diffuse;

      // specular
      vec3 specular = vec3(0.0f);
      vec3 R = normalize(reflect(-L, N));
      vec3 E = normalize(cameraPos - posWorld);
      float RdotE = clamp(dot(R, E), 0.0f, 1.0f);
      if (RdotE > 0.0f) {
         specular = ks * pow(RdotE, shininess);
      }
      illumination += visibility * attenuation * light.color * specular;
   }

   fragColor = vec4(illumination, 1.0f);

   // float h = texture(heightMap, texCoord).r;
   // fragColor = vec4(vec3(h), 1.0); // show height as grayscale
}
