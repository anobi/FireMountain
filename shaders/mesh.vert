#  version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

layout (location = 0) out vec3 outColor;

layout (push_constant) uniform constants {
    vec4 data;
    mat4 render_matrix;
} pushConstants;

void main() {
    gl_Position = pushConstants.render_matrix * vec4(inPosition, 1.0f);
    outColor = inColor;
}