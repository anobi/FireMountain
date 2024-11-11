#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "input_structures.glsl"

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outWorldPosition;
layout (location = 4) out vec4 outTangent;

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 color;
    vec4 tangent;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout (push_constant) uniform constants {
    mat4 model_matrix;
    VertexBuffer vertex_buffer;
} PushConstants;

void main() {
    Vertex vert = PushConstants.vertex_buffer.vertices[gl_VertexIndex];

    mat4 m = PushConstants.model_matrix;
    mat4 v = sceneData.viewMatrix;
    mat4 p = sceneData.projectionMatrix;
    mat4 vp = p * v;
    mat4 mvp = vp * m;

    // TODO: Calculate TBN here too

    vec4 position = vec4(vert.position, 1.0f);
    vec4 worldPos = m * position;
    vec4 frag_pos = vp * worldPos;
    gl_Position = frag_pos;


    outWorldPosition = worldPos.xyz;
    outNormal = vec3(m * vec4(vert.normal, 0.0f)).xyz;
    outColor = vert.color.xyz * materialData.colorFactors.xyz;
    outUV.x = vert.uv_x;
    outUV.y = vert.uv_y;
    outTangent = vert.tangent;
}