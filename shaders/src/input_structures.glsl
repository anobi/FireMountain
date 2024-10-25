#extension GL_GOOGLE_include_directive : require
#include "light.glsl"


layout(set = 0, binding = 0) uniform SceneData {
    mat4 viewProjection;
    vec4 cameraPosition;

    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;

    Light lights[32];

} sceneData;

layout(set = 1, binding = 0) uniform GLTFMaterialData {
    vec4 colorFactors;
    vec4 metalRoughFactors;
} materialData;

layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;