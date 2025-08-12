struct Vertex
{
    float3 position;
    float uv_x;
    float3 normal;
    float uv_y;
    float4 color;
    float4 tangent;
};

struct Light
{
    float4 position_type;
    float4 color_intensity;
    float4 direction_range;
    float4 info;
};

struct PushConstants
{
    float4x4 model_matrix;
    Vertex* vertex_buffer;
};
[[vk::push_constant]] PushConstants push_constants;

struct SceneData
{
    float4x4 view_matrix;
    float4x4 projection_matrix;
    float3 camera_position;
    uint light_count;
    Light lights[32];
};
[[vk::binding(0)]] SceneData scene_data;
[[vk::binding(1)]] Sampler2D textures[];

struct Output {
    float4 position : SV_Position;
    float2 uv;
    float3 color;
}

[shader("vertex")]
Output vsMain(uint vertexID : SV_VertexID)
{
       Output output;
       Vertex vert = push_constants.vertex_buffer[vertexID];

       float4x4 m = push_constants.model_matrix;
       float4x4 v = scene_data.view_matrix;
       float4x4 p = scene_data.projection_matrix;
       float4x4 vp = p * v;
       float4x4 mvp = vp * m;

       float4 position = float4(vert.position, 1.0f);
       float4 worldPos = mul(m, position);
       float4 frag_pos = mul(vp, worldPos);

       output.position = frag_pos;
       output.uv = float2(vert.uv_x, vert.uv_y);
       output.color = vert.color.xyz;

       // Output built-in is returned from the function

       return output;
}

[shader("pixel")]
float4 psMain(Output input) : SV_Target
{
    return float4(0.4, 0.4, 0.4, 1.0);
}