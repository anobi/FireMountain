#version 450

// Input
layout (location = 0) in vec3 in_Color;

// Output
layout (location = 0) out vec4 out_FragColor;


void main() {
    out_FragColor = vec4(in_Color, 1.0f);
}