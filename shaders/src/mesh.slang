// mesh.slang

struct Light
{
    float4 position_type;
    float4 color_intensity;
    float4 direction_range;
    float4 info;
};

struct Vertex
{
    float3 position;
    float uv_x;
    float3 normal;
    float uv_y;
    float4 color;
    float4 tangent;
};

struct SceneData
{
    float4x4 view_matrix;
    float4x4 projection_matrix;
    float3 camera_position;
    uint light_count;
    Light lights[32];
};
layout(set = 0, binding = 0) ConstantBuffer<SceneData> scene_data;
layout(set = 0, binding = 1) ConstantBuffer<Sampler2D> textures[];

struct GLTFmaterial_data
{
    float4 color_factors;
    float2 metal_rough_factors;
    bool has_metal_roughness_map;
    bool has_color_map;

    float4 emissive_factor;
    bool use_alpha_blending;
    bool has_emissive_map;
    bool has_normal_map;

    int color_texture_id;
    int metal_roughness_texture_id;
    int normal_texture_id;
    int emissive_texture_id;
};
layout(set = 1, binding = 0) ConstantBuffer<GLTFmaterial_data> material_data;

struct PushConstants
{
    float4x4 model_matrix;
    Vertex* vertex_buffer;
};
[[vk::push_constant]] ConstantBuffer<PushConstants> push_constants;

struct VertexStageOutput
{
    float4 position : SV_Position;
    float3 normal;
    float3 color;
    float2 uv;
    float3 world_position;
    float4 tangent;
};

[shader("vertex")]
VertexStageOutput vsMain(uint vertexID : SV_VertexID)
{
    VertexStageOutput output;
    Vertex vert = push_constants.vertex_buffer[vertexID];

    float4x4 m = push_constants.model_matrix;
    float4x4 v = scene_data.view_matrix;
    float4x4 p = scene_data.projection_matrix;
    float4x4 vp = p * v;
    float4x4 mvp = vp * m;

    // TODO: Calculate TBN here too

    float4 position = float4(vert.position, 1.0f);
    float4 worldPos = mul(m, position);
    float4 frag_pos = mul(vp, worldPos);

    output.position = frag_pos;
    output.world_position = worldPos.xyz;
    output.normal = mul(m, float4(vert.normal, 0.0f)).xyz;
    output.color = mul(vert.color.xyz, material_data.color_factors.xyz);
    output.uv.x = vert.uv_x;
    output.uv.y = vert.uv_y;
    output.tangent = vert.tangent;

    return output;
}

//
// Pixel Shader
//

static float GAMMA = 2.2;
static float PI = 3.14159265359;

float3 F_Schlick(const float3 f0, float f90, float u) {
    return f0 + (float3(f90) - f0) * pow(1.0 - u, 5.0);
}

float3 F_Schlick(const float3 f0, float u) {
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

float D_GGX(float NdotH, float roughness, const float3 n, const float3 h) {
    float3 NxH   = cross(n, h);
    float a    = NdotH * roughness;
    float k    = roughness / (dot(NxH, NxH) + a * a);
    float d    = k * k * (1.0 / PI);
    return min(d, 65504.0);
}

float3 pointLight(uint index, float3 normal, float3 world_position) {
    float3 worldToLight = scene_data.lights[index].position_type.xyz - world_position;
    float dist = length(worldToLight);
    float attenuation = 1.0 / (dist * dist);

    worldToLight = normalize(worldToLight);
    float NdotL = clamp(dot(normal, worldToLight), 0.0, 1.0);
    return NdotL
        * scene_data.lights[index].color_intensity.w
        * attenuation
        * scene_data.lights[index].color_intensity.xyz;
}

float3 directionalLight(uint index, float3 normal) {
    float3 worldToLight = normalize(-scene_data.lights[index].direction_range.xyz);
    float NdotL = clamp(dot(normal, worldToLight), 0.0, 1.0);
    return NdotL
        * scene_data.lights[index].color_intensity.w
        * scene_data.lights[index].color_intensity.xyz;
}

float3 getLightDirection(uint index, float3 world_position) {
    // Point light
    if (scene_data.lights[index].position_type.w == 1.0) {
        return scene_data.lights[index].position_type.xyz - world_position.xyz;
    }
    // Directional light
    else {
    // if (sceneData.lights[index].positionType.w == 0.0) {
        return -scene_data.lights[index].direction_range.xyz;
    }
}

float3 normal(VertexStageOutput input) {
    float3 posDx  = ddx(input.world_position);
    float3 posDy  = ddy(input.world_position);
    float3 st1    = ddx(float3(input.uv, 0.0));
    float3 st2    = ddy(float3(input.uv, 0.0));

    float3 T = input.tangent.xyz;
    float flip = input.tangent.w;

    // Calculate tangent if one isn't found in inputs
    if (length(input.tangent) == 0.0) {
        T = (st2.y * posDx - st1.y * posDy) / (st1.x * st2.y - st2.x * st1.y);
        float uv2xArea = st1.x * st2.y - st1.y * st2.x;
        flip = uv2xArea > 0 ? 1 : -1;
    }

    float3 N      = normalize(input.normal);
    T           = normalize(T - N * dot(N, T));
    float3 B      = normalize(cross(N, T) * flip);
    float3x3 TBN    = float3x3(T, B, N);

    if (material_data.has_normal_map) {
       // float4 normalMap = textures[material_data.normal_texture_id].Sample(input.uv);
        float4 normalMap = float4(1.0, 1.0, 1.0, 1.0);
        return normalize(mul(TBN, (2.0 * normalMap.xyz - 1.0)));
    }
    return normalize(TBN[2].xyz);
}

float addEmissive(inout float4 color, float2 uv) {
    float4 emissive = float4(0.0);
    if (material_data.has_emissive_map) {
        // emissive = textures[material_data.emissive_texture_id].Sample(uv) * material_data.emissive_factor;
        emissive = float4(1.0, 1.0, 1.0, 1.0);
    }
    else {
        emissive = material_data.emissive_factor;
    }

    float is_emissive = length(emissive) > 1.0 ? 1.0 : 0.0;
    if (is_emissive == 1.0) {
        // Transparency / fade thing
        float exposure = 1.0; // Placeholder basically
        float attenuation = lerp(1.0, exposure, emissive.w);

        emissive = pow(emissive, float4(GAMMA));
        attenuation *= color.w;
        color.xyz += emissive.xyz * attenuation;
    }

    return is_emissive;
}

[shader("pixel")]
float4 psMain(VertexStageOutput input) : SV_Target
{
    float4 base_color = float4(1.0, 0.0, 0.0, 1.0);

    return float4(1, 1, 1, 1);

    if (material_data.has_color_map) {
        // float4 tex_color = textures[material_data.color_texture_id].Sample(input.uv);
        float4 tex_color = float4(1.0, 0.0, 0.0, 1.0);
        base_color = mul(tex_color, material_data.color_factors);
        // Convert to linear color space if in srgb
        base_color = pow(base_color, float4(GAMMA));
    }
    else {
        base_color = float4(input.color, 1.0) * material_data.color_factors;
    }

    if (base_color.w == 0.0) {
        // discard;
    }

    // Do we need to properly alpha blend things or not?
    float alpha = (material_data.use_alpha_blending) ? base_color.w : 1.0;
    float metallic = 0.04;
    float roughness = 0.8;
    if (material_data.has_metal_roughness_map) {
        // float4 metalRough = textures[material_data.metal_roughness_texture_id].Sample(input.uv);
        float4 metalRough = float4(metallic, roughness, 0.0, 1.0);
        metallic = clamp(metalRough.x, 0.0, 1.0);
        roughness = clamp(metalRough.y, 0.0, 1.0);
    }
    else {
        metallic = material_data.metal_rough_factors.x;
        roughness = material_data.metal_rough_factors.y;
    }
    roughness = roughness * roughness;

    // Calculate lights
    float3 n = normal(input);  // Normal unit vector
    float3 v = normalize(scene_data.camera_position.xyz - input.world_position);  // View unit vector
    float3 f0 = 0.16 * roughness * roughness * (1.0 - metallic) + base_color.xyz * metallic;
    float f90 = clamp(dot(f0, float3(50.0 * 0.33)), 0.0, 1.0);


    float4 lightValue = float4(float3(0.0), base_color.w);
    float3 diffuseColor = (1.0 - metallic) * base_color.xyz;
    for (uint i = 0; i < scene_data.light_count; i++) {
        float3 l = getLightDirection(i, input.world_position);
        float3 h = normalize(v + l);
        float NoV = max(dot(n, v), 1e-4);
        float NoL = clamp(dot(n, l), 0.0, 1.0);
        float NoH = clamp(dot(n, h), 0.0, 1.0);
        float LoH = clamp(dot(l, h), 0.0, 1.0);

        float D = D_GGX(NoH, roughness, n, h);
        float3 F = F_Schlick(f0, f90, LoH);
        float V = V_SmithGGXCorrelated(NoV, NoL, roughness);

        // Specular BRDF
        float3 Fr = (D * V) * F;

        // Diffuse BRDF
        float3 Fd = diffuseColor * Fd_Burley(NoV, NoL, LoH, roughness);
        // float3 Fd = diffuseColor * Fd_Lambert();
        float3 shading = Fd + Fr;

        // Apply point lights
        if (scene_data.lights[i].position_type.x == 1.0) {
            lightValue.xyz.add(pointLight(i, n, input.world_position) * shading);
        }
        // Apply directional lighting
        else {
            lightValue.xyz.add(directionalLight(i, n) * shading);
        }
    }

    float is_emissive = addEmissive(lightValue, input.uv);
    float3 irradiance = float3(0.1);
    float3 iblDiffuse = irradiance * base_color.xyz;
    float3 ambient = iblDiffuse;
    float3 color = float3(ambient + lightValue.xyz);

    // Apply gamma correction
    if (is_emissive == 0.0) {
        color = color / (color + float3(1.0f));
        color = pow(color, float3(1.0f / GAMMA));
    }

    return float4(color, alpha);
    //return float4(1, 1, 1, 1);
}