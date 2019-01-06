#version 440

#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 projection;
} mvp;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;
layout(location = 5) in vec3 inColor;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragTangent;
layout(location = 3) out vec3 fragBitangent;
layout(location = 4) out vec3 fragColor;

void main()
{
   gl_Position = mvp.projection * mvp.view * mvp.model * vec4(inPosition, 1.0);
   // fragNormal
   fragTexCoord = inTexCoord;
   // fragTangent
   // fragBitangent
   // fragColor
}