#version 450

void main() {
    const vec3 position[3] = vec3[3](
        vec3( 1.0f,  1.0f, 0.0f),
        vec3(-1.0f,  1.0f, 0.0f),
        vec3( 1.0f, -1.0f, 0.0f)
    );
    gl_Position = vec4(position[gl_VertexIndex], 1.0f);
}