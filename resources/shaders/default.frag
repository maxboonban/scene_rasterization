#version 330 core

in vec3 posWorld;
in vec3 normWorld;
in vec4 posLightSpace[8];
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

uniform sampler2D shadowMaps[8];
uniform int shadowSize;
uniform bool softShadows;

void main() {
    // normalize normals
   vec3 N = normalize(normWorld);

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
}
