#version 440

layout(binding = 0) uniform ModelTransform {
    mat4 model;
    mat4 mInvTranspose;
} modelTransform;

layout(binding = 1) uniform ViewProjectionTransform {
    mat4 view;
    mat4 projection;
} dynamicVP;

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
    gl_Position = dynamicVP.projection * dynamicVP.view * modelTransform.model * position;

    vs_out.worldSpacePos = (modelTransform.model * position).xyz;
    vs_out.worldSpaceNormal = (modelTransform.mInvTranspose * vec4(inNormal, 0.0)).xyz;
    vs_out.texCoords = inTexCoord;
}