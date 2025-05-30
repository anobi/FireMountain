#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#define USE_BINDLESS
#include "input_structures.glsl"

// Input
layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inWorldPosition;
layout (location = 4) in vec4 inTangent;

// Output
layout (location = 0) out vec4 outFragColor;

const float GAMMA = 2.2;
const float PI = 3.14159265359;

vec3 F_Schlick(const vec3 f0, float f90, float u) {
    return f0 + (vec3(f90) - f0) * pow(1.0 - u, 5.0);
}

vec3 F_Schlick(const vec3 f0, float u) {
    float f = pow(1.0 - u, 5.0);
    return f + f0 * (1.0 - f);
}

float F_Schlick(float f0, float f90, float u) {
    return f0 + (f90 - f0) * pow(1.0 - u, 5.0);
}

float Fd_Lambert() {
    return 1.0 / PI;
}

float Fd_Burley(float NoV, float NoL, float LoH, float roughness) {
    float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
    float lightScatter  = F_Schlick(1.0, f90, NoL);
    float viewScatter   = F_Schlick(1.0, f90, NoV);
    return lightScatter * viewScatter * (1.0 / PI);
}

float V_SmithGGXCorrelated(float NoV, float NoL, float roughness) {
    float a2 = roughness * roughness;
    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5 / (GGXV + GGXL);
}

// Faster, mathematically wrong approximation for mobile etc.
float V_SmithGGXCorrelatedFast(float NdotV, float NdotL, float roughness) {
    float a = roughness;
    float GGXV = NdotL * (NdotV * (1.0 - a) + a);
    float GGXL = NdotV * (NdotL * (1.0 - a) + a);
    return 0.5 / (GGXV + GGXL);
}

float D_GGX(float NdotH, float roughness) {
    float a    = NdotH * roughness;
    float k    = roughness / (1.0 - NdotH * NdotH + a * a);
    float d    = k * k * (1.0 / PI);
    return d;
}

float D_GGX(float NdotH, float roughness, const vec3 n, const vec3 h) {
    vec3 NxH   = cross(n, h);
    float a    = NdotH * roughness;
    float k    = roughness / (dot(NxH, NxH) + a * a);
    float d    = k * k * (1.0 / PI);
    return min(d, 65504.0);
}

vec3 pointLight(uint index, vec3 normal) {
    vec3 worldToLight = sceneData.lights[index].positionType.xyz - inWorldPosition.xyz;
    float dist = length(worldToLight);
    float attenuation = 1.0 / (dist * dist);

    worldToLight = normalize(worldToLight);
    float NdotL = clamp(dot(normal, worldToLight), 0.0, 1.0);
    return NdotL * sceneData.lights[index].colorIntensity.w * attenuation * sceneData.lights[index].colorIntensity.rgb;
}

vec3 directionalLight(uint index, vec3 normal) {
    vec3 worldToLight = normalize(-sceneData.lights[index].directionRange.xyz);
    float NdotL = clamp(dot(normal, worldToLight), 0.0, 1.0);
    return NdotL * sceneData.lights[index].colorIntensity.w * sceneData.lights[index].colorIntensity.rgb;
}

vec3 getLightDirection(uint index) {
    // Point light
    if (sceneData.lights[index].positionType.w == 1.0) {
        return sceneData.lights[index].positionType.xyz - inWorldPosition.xyz;
    }
    // Directional light
    else {
    // if (sceneData.lights[index].positionType.w == 0.0) {
        return -sceneData.lights[index].directionRange.xyz;
    }
}

vec3 normal() {
    vec3 posDx  = dFdx(inWorldPosition);
    vec3 posDy  = dFdy(inWorldPosition);
    vec3 st1    = dFdx(vec3(inUV, 0.0));
    vec3 st2    = dFdy(vec3(inUV, 0.0));

    vec3 T = inTangent.xyz;
    float flip = inTangent.w;

    // Calculate tangent if one isn't found in inputs
    if (length(inTangent) == 0.0) {
        T = (st2.t * posDx - st1.t * posDy) / (st1.s * st2.t - st2.s * st1.t);
        float uv2xArea = st1.x * st2.y - st1.y * st2.x;
        flip = uv2xArea > 0 ? 1 : -1;
    }

    vec3 N      = normalize(inNormal);
    T           = normalize(T - N * dot(N, T));
    vec3 B      = normalize(cross(N, T) * flip);
    mat3 TBN    = mat3(T, B, N);

    if (materialData.hasNormalMap == 1.0) {
        vec4 normalMap = texture(textures[materialData.normalTexID], inUV);
        return normalize(TBN * (2.0 * normalMap.rgb - 1.0));
    }
    return normalize(TBN[2].xyz);
}

float addEmissive(inout vec4 color) {
    vec4 emissive = vec4(0.0);
    if (materialData.hasEmissiveMap == 1.0) {
        emissive = texture(textures[materialData.emissiveTexID], inUV) * materialData.emissiveFactor;
    }
    else {
        emissive = materialData.emissiveFactor;
    }

    float is_emissive = length(emissive) > 1.0 ? 1.0 : 0.0;
    if (is_emissive == 1.0) {
        // Transparency / fade thing
        float exposure = 1.0; // Placeholder basically
        float attenuation = mix(1.0, exposure, emissive.w);

        emissive = pow(emissive, vec4(GAMMA));
        attenuation *= color.a;
        color.rgb += emissive.rgb * attenuation;
    }

    return is_emissive;
}

void main() {
    vec4 baseColor = vec4(1.0, 0.0, 0.0, 1.0);
    if (materialData.hasColorMap == 1.0) {
        baseColor = texture(textures[materialData.colorTexID], inUV) * materialData.colorFactors;
        // Convert to linear color space if in srgb
        baseColor = pow(baseColor, vec4(GAMMA));
    }
    else {
        baseColor = vec4(inColor, 1.0) * materialData.colorFactors;
    }

    if (baseColor.a == 0.0) {
        discard;
    }

    // Do we need to properly alpha blend things or not?
    float alpha = (materialData.useAlphaBlending == 1.0) ? baseColor.a : 1.0;
    float metallic = 0.04;
    float roughness = 0.8;
    if (materialData.hasMetalRoughnessMap == 1.0) {
        vec4 metalRough = texture(textures[materialData.metalRoughTexID], inUV);
        metallic = clamp(metalRough.r, 0.0, 1.0);
        roughness = clamp(metalRough.g, 0.0, 1.0);
    }
    else {
        metallic = materialData.metalRoughFactors.x;
        roughness = materialData.metalRoughFactors.y;
    }
    roughness = roughness * roughness;

    // Calculate lights
    vec3 n = normal();  // Normal unit vector
    vec3 v = normalize(sceneData.cameraPosition.xyz - inWorldPosition);  // View unit vector
    vec3 f0 = 0.16 * roughness * roughness * (1.0 - metallic) + baseColor.rgb * metallic;
    float f90 = clamp(dot(f0, vec3(50.0 * 0.33)), 0.0, 1.0);

    vec4 lightValue = vec4(vec3(0.0), baseColor.a);
    vec3 diffuseColor = (1.0 - metallic) * baseColor.rgb;
    for (uint i = 0; i < sceneData.lightCount; i++) {
        vec3 l = getLightDirection(i);
        vec3 h = normalize(v + l);
        float NoV = max(dot(n, v), 1e-4);
        float NoL = clamp(dot(n, l), 0.0, 1.0);
        float NoH = clamp(dot(n, h), 0.0, 1.0);
        float LoH = clamp(dot(l, h), 0.0, 1.0);

        float D = D_GGX(NoH, roughness, n, h);
        vec3 F = F_Schlick(f0, f90, LoH);
        float V = V_SmithGGXCorrelated(NoV, NoL, roughness);

        // Specular BRDF
        vec3 Fr = (D * V) * F;

        // Diffuse BRDF
        vec3 Fd = diffuseColor * Fd_Burley(NoV, NoL, LoH, roughness);
        // vec3 Fd = diffuseColor * Fd_Lambert();
        vec3 shading = Fd + Fr;

        // Apply point lights
        if (sceneData.lights[i].positionType.w == 1.0) {
            lightValue.rgb += pointLight(i, n) * shading;
        } 
        // Apply directional lighting 
        else {
            lightValue.rgb += directionalLight(i, n) * shading;
        }
    }

    float is_emissive = addEmissive(lightValue);
    vec3 irradiance = vec3(0.1);
    vec3 iblDiffuse = irradiance * baseColor.rgb;
    vec3 ambient = iblDiffuse;
    vec3 color = vec3(ambient + lightValue.rgb);

    // Apply gamma correction
    if (is_emissive == 0.0) {
        color = color / (color + vec3(1.0f));
        color = pow(color, vec3(1.0f / GAMMA));
    }

    outFragColor = vec4(color.rgb, alpha);
}