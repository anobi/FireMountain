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

const float PI = 3.14159265359;
vec3 F0 = vec3(0.04f);


vec3 fresnelSchlick(vec3 f0, float f90, float u) {
    return f0 + (f90 - f0) * pow(1.0 - u, 5.0);
}

vec3 fresnelSchlickRoughness(vec3 f0, float cosTheta, float roughness) {
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(1.0f - cosTheta, 5.0f);
}

float diffuseTerm(float NdotV, float NdotL, float LdotH, float roughness) {
    float eBias         = 0.0 * (1.0 - roughness) + 5.0 * roughness;
    float eFactor       = 1.0 * (1.0 - roughness) + (1.0 / 1.51) * roughness;
    float fd90          = eBias + 2.0 * LdotH * LdotH * roughness;
    vec3 f0             = vec3(1.0);

    float lightScatter  = fresnelSchlick(f0, fd90, NdotL).r;
    float viewScatter   = fresnelSchlick(f0, fd90, NdotV).r;
    return lightScatter * viewScatter * eFactor;
}

float SmithGGXCorrelated(float NdotV, float NdotL, float roughness) {
    float alphaRoughessSq = roughness * roughness;
    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughessSq) + alphaRoughessSq);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughessSq) + alphaRoughessSq);
    float GGX = GGXV + GGXL;
    if (GGX > 0.0) {
        return 0.5 / GGX;
    }
    return 0.0;
}

float distributionGGX(float NdotH, float roughness) {
    float a2    = roughness * roughness;
    float f     = (NdotH * a2 - NdotH) * NdotH + 1.0;
    return a2 / (PI * f * f);
}

vec3 pointLight(uint index, vec3 normal) {
    vec3 worldToLight = sceneData.lights[index].positionType.xyz - inWorldPosition.xyz;
    float dist = length(worldToLight);
    float attenuation = 1.0f / (dist * dist);

    worldToLight = normalize(worldToLight);
    float NdotL = clamp(dot(normal, worldToLight), 0.0f, 1.0f);
    return NdotL * sceneData.lights[index].colorIntensity.w * attenuation * sceneData.lights[index].colorIntensity.rgb;
}

vec3 directionalLight(uint index, vec3 normal) {
    vec3 worldToLight = normalize(-sceneData.lights[index].directionRange.xyz);
    float NdotL = clamp(dot(normal, worldToLight), 0.0f, 1.0f);
    return NdotL * sceneData.lights[index].colorIntensity.w * sceneData.lights[index].colorIntensity.rgb;
}

vec3 getLightDirection(uint index) {
    // Directional light
    if (sceneData.lights[index].positionType.w == 0.0) {
        return -sceneData.lights[index].directionRange.xyz;
    }
    // Point light
    if (sceneData.lights[index].positionType.w == 1.0) {
        return sceneData.lights[index].directionRange.xyz - inWorldPosition.xyz;
    }
}

vec3 normal() {
    vec3 posDx  = dFdx(inWorldPosition);
    vec3 posDy  = dFdy(inWorldPosition);
    vec3 st1    = dFdx(vec3(inUV, 0.0));
    vec3 st2    = dFdy(vec3(inUV, 0.0));
    vec3 T      = (st2.t * posDx - st1.t * posDy) / (st1.s * st2.t - st2.s * st1.t);
    vec3 N      = normalize(inNormal);
    T           = normalize(T - N * dot(N, T));
    vec3 B      = normalize(cross(N, T));
    mat3 TBN    = mat3(T, B, N);

    vec3 n = texture(normalTex, inUV).rgb;
    return normalize(TBN * (2.0 * n - 1.0));
}


void main() {
    float gamma = 2.0;
    float F90 = clamp(50.0 * F0.r, 0.0, 1.0);

    float metallic = 0.04;
    float roughness = 0.8f;
    // float metallic = materialData.metalRoughFactors.x;
    // float roughness = materialData.metalRoughFactors.y;
    vec4 baseColor = vec4(1.0, 0.0, 0.0, 1.0);

    // TODO: separate to texture and base color based on if material has texture or not
    // baseColor = vec4(inColor, 1.0) * texture(colorTex, inUV);
    baseColor = texture(colorTex, inUV);

    // Convert to linear color space
    baseColor = pow(baseColor, vec4(gamma)); 


    // Calculate lights
    vec3 N = normal();  // Normal unit vector
    vec3 V = normalize(sceneData.cameraPosition.xyz - inWorldPosition);  // View unit vector
    float NdotV = clamp(dot(N, V), 0.0f, 1.0f);
    vec3 lightValue = vec3(0.0f);
    vec3 diffuseColor = baseColor.rgb * (1.0f - metallic);

    for (uint i = 0; i < sceneData.lightCount; i++) {
        vec3 L = getLightDirection(i);
        vec3 H = normalize(V + L);  // Half vector
        float LdotH = clamp(dot(L, H), 0.0f, 1.0f);
        float NdotH = clamp(dot(N, H), 0.0f, 1.0f);
        float NdotL = clamp(dot(N, L), 0.0f, 1.0f);

        vec3 F = fresnelSchlick(F0, F90, LdotH);
        float Vis = SmithGGXCorrelated(NdotV, NdotL, roughness);
        float D = distributionGGX(NdotH, roughness);
        vec3 Fr = F * D * Vis;

        float Fd = diffuseTerm(NdotV, NdotL, LdotH, roughness);

        if (sceneData.lights[i].positionType.w == 0.0) {
            lightValue += directionalLight(i, N) * diffuseColor * (vec3(1.0) - F) * Fd + Fr;
        }
        if (sceneData.lights[i].positionType.w == 1.0) {
            lightValue += pointLight(i, N) * diffuseColor * (vec3(1.0) - F) * Fd + Fr;
        }
    }

    vec3 irradiance = vec3(0.5);
    vec3 F = fresnelSchlickRoughness(F0, max(dot(N, V), 0.0), roughness * roughness * roughness * roughness);
    vec3 iblDiffuse = irradiance * baseColor.rgb;
    vec3 ambient = iblDiffuse;

    vec3 color = vec3(0.3 * ambient + lightValue);
    // color = color / (color + vec3(1.0f));
    color = pow(color, vec3(1.0f / gamma));

    outFragColor = vec4(color, baseColor.a);
}