#version 440

#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 projection;
} mvp;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main()
{
   gl_Position = mvp.projection * mvp.view * mvp.model * vec4(inPosition, 1.0);
   fragColor = inColor;
}