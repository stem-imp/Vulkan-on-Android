#version 440

layout(binding = 0) uniform ModelTransform {
    mat4 model;
} modelTransform;

layout(binding = 1) uniform ViewProjectionTransform {
    mat4 view;
    mat4 projection;
} dynamicVP;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out VS_OUT {
    vec3 lightDirInTangentSpace;
    vec3 viewDirInTangentSpace;
    vec2 texCoords;
} vs_out;

void main()
{
    vec4 position = vec4(inPosition, 1.0);
    gl_Position = dynamicVP.projection * dynamicVP.view * modelTransform.model * position;

    mat4 mv = dynamicVP.view * modelTransform.model;

    // push constants
    vec3 lightPosInWorldSpace = vec3(12, 12, 12);
    vec3 cameraPosInWorldSpace = vec3(0, 0, 4);

    vec3 posInCameraSpace = (mv * vec4(inPosition, 1.0)).xyz;
    vec3 lightPosInCameraSpace = (dynamicVP.view * vec4(lightPosInWorldSpace, 1)).xyz;
    vec3 lightDirInCameraSpace = normalize(lightPosInCameraSpace - posInCameraSpace);

    vec3 viewDirInCameraSpace = normalize(cameraPosInWorldSpace - posInCameraSpace);

    vec3 tangentInCameraSpace = (mv * normalize(vec4(inTangent, 1.0))).xyz;
    vec3 bitangentInCameraSpace = (mv * normalize(vec4(inBitangent, 1.0))).xyz;
    vec3 normalInCameraSpace = normalize((transpose(inverse(mv)) * vec4(inNormal, 1.0)).xyz);
    mat3 toTangentSpace = transpose(mat3(tangentInCameraSpace, bitangentInCameraSpace, normalInCameraSpace));

    vs_out.lightDirInTangentSpace = toTangentSpace * lightDirInCameraSpace;
    vs_out.viewDirInTangentSpace = toTangentSpace * viewDirInCameraSpace;
    vs_out.texCoords = inTexCoord;
}