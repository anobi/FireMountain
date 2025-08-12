// bg_gradient.slang

layout(binding = 0) RWTexture2D<float4> outputImage;
struct PushConstants
{
    float4 data_1;
    float4 data_2;
    float4 data_3;
    float4 data_4;
};
layout(push_constant) ConstantBuffer<PushConstants> push_constants;

float4 debanding(float4 col, int2 uv)
{
    float noise_scale = 0.003173828125;
    float rand = fract(sin(dot(uv.xy, float2(12.9898, 78.233))) * 43758.5453);
    return clamp(col + rand * noise_scale - noise_scale / 2.0, 0.0, 1.0);
}

[shader("compute")]
[require(spirv_1_4)]
[numthreads(16,16,1)]
void csMain(uint3 threadId : SV_DispatchThreadID)
{
    int2 size;
    outputImage.GetDimensions(size.x, size.y);
    int2 texel_coord = threadId.xy;
    float4 top_color = push_constants.data_1;
    float4 bottom_color = push_constants.data_2;

    if(texel_coord.x < size.x && texel_coord.y < size.y) {
        float blend = (float)texel_coord.y / size.y;
        float brightness = 0.85;

        float4 out_color = lerp(top_color, bottom_color, blend) * brightness;

        // Add some noise to create texture
        // This looks terrible, why?
        out_color += 0.015 * fract(sin(dot(texel_coord, float2(12.9898, 78.233))) * 43758.5453);

        // Tonemapping and debanding
        out_color = 1.0f - exp(-out_color);
        out_color = debanding(out_color, texel_coord);
        outputImage[texel_coord] = out_color;
    }
}
