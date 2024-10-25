#version 450

layout (location = 0) out vec3 out_Color;

void main() {
    const vec3 position[3] = vec3[3](
        vec3( 1.0f,  1.0f, 0.0f),
        vec3(-1.0f,  1.0f, 0.0f),
        vec3( 0.0f, -1.0f, 0.0f)
    );

    const vec3 colors[3] = vec3[3](
        vec3(1.0f, 0.0f, 0.0f),
        vec3(0.0f, 1.0f, 0.0f),
        vec3(0.0f, 0.0f, 1.0f)
    );

    gl_Position = vec4(position[gl_VertexIndex], 1.0f);
    out_Color = colors[gl_VertexIndex];
}