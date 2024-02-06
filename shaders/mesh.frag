#version 450

// Input
layout (location = 0) in vec4 inColor;

// Output
layout (location = 0) out vec4 outFragColor;


void main() {
    outFragColor = vec4(inColor);
}