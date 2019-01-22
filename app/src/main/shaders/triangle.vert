/*#version 440

#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform MVP {
    mat4 model;
} mvp;

layout(binding = 1) uniform ViewProjectionTransform {
    mat4 view;
    mat4 projection;
} dynamicVP;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main()
{
   gl_Position = dynamicVP.projection * dynamicVP.view * mvp.model * vec4(inPosition, 1.0);
   fragColor = inColor;
   fragTexCoord = inTexCoord;
}*/