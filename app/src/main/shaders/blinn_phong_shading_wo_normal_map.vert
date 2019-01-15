#version 440

layout(binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 mInvTranspose;
} mvp;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out VS_OUT {
    vec3 worldSpacePos;
    vec3 worldSpaceNormal;
    vec2 texCoords;
} vs_out;

void main()
{
    vec4 position = vec4(inPosition, 1.0);
    gl_Position = mvp.projection * mvp.view * mvp.model * position;

    vs_out.worldSpacePos = (mvp.model * position).xyz;
    vs_out.worldSpaceNormal = (mvp.mInvTranspose * vec4(inNormal, 0.0)).xyz;
    vs_out.texCoords = inTexCoord;
}