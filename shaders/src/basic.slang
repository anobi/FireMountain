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

    // Oh shit the problem was doint p * v for matrix multiplication instead of mul(p, v)
    float4x4 vp = mul(p, v);
    float4x4 mvp = mul(vp, m);

    float4 position = float4(vert.position, 1.0f);
    float4 worldPos = mul(m, position);
    float4 frag_pos = mul(vp, worldPos);

    output.position = frag_pos;
    output.color = vert.color.xyz;
    return output;
}

[shader("pixel")]
float4 psMain(VertexStageOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}