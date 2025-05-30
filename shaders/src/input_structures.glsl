#extension GL_GOOGLE_include_directive : require
#include "light.glsl"


layout(set = 0, binding = 0) uniform SceneData {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec3 cameraPosition;

    uint lightCount;
    Light lights[32];

} sceneData;

#ifdef USE_BINDLESS
layout(set = 0, binding = 1) uniform sampler2D textures[];
#else
layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;
layout(set = 1, binding = 3) uniform sampler2D normalTex;
layout(set = 1, binding = 4) uniform sampler2D emissiveTex;
#endif

layout(set = 1, binding = 0) uniform GLTFMaterialData {
    vec4 colorFactors;
    vec2 metalRoughFactors;
    float hasMetalRoughnessMap;
    float hasColorMap;

    vec4 emissiveFactor;
    float useAlphaBlending;
    float hasEmissiveMap;
    float hasNormalMap;

    int colorTexID;
    int metalRoughTexID;
    int normalTexID;
    int emissiveTexID;
} materialData;