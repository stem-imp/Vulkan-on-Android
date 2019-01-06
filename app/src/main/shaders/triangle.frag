#version 440
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D normalSampler;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(0.0, 1.0, 0.75, 1.0);
}