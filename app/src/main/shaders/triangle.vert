#version 440

#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 projection;
} mvp;

layout(location = 0) in vec3 inPosition;

void main()
{
   gl_Position = mvp.projection * mvp.view * mvp.model * vec4(inPosition, 1.0);
}