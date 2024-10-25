#version 450

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"

// Input
layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inWorldPosition;

// Output
layout (location = 0) out vec4 outFragColor;

void main() {
    float light_value = max(dot(inNormal, sceneData.sunlightDirection.xyz), 0.1f);
    vec3 color = inColor * texture(colorTex, inUV).xyz;
    vec3 ambient = color * sceneData.ambientColor.xyz;

    vec3 N = normalize(inNormal);  // Normal unit vector
    vec3 V = normalize(sceneData.cameraPosition.xyz - inWorldPosition);  // View unit vector
    for (int i = 0; i < 32; i++) {
        if (sceneData.lights[i].positionType.w == 0.0f) {
            continue;
        }
        vec3 L = normalize(sceneData.lights[i].positionType.xyz - inWorldPosition);  // Light unit vector
        vec3 H = normalize(V + L);  // Half vector
        float d = distance(sceneData.lights[i].positionType.xyz, inWorldPosition);
        float cosTheta = max(dot(N, L), 0.0);
        float attenuation = 1.0f / (d * d);
        vec3 radiance = sceneData.lights[i].colorIntensity.xyz * attenuation * cosTheta;
        color += radiance * sceneData.lights[i].colorIntensity.w;
    }

    outFragColor = vec4(color * light_value * sceneData.sunlightColor.w + ambient, 1.0f);
}