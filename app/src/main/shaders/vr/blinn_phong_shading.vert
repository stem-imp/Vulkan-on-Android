#version 440

layout(binding = 0) uniform ModelTransform {
    mat4 model;
} modelTransform;

layout(binding = 1) uniform ViewProjectionTransform {
    mat4 view;
    mat4 projection;
} dynamicVP;

layout(push_constant) uniform Lighting {
	vec3 cameraPosInWorldSpace;
	layout(offset = 16) vec3 lightPosInWorldSpace;
	layout(offset = 32) vec3 ambientLight;
	layout(offset = 48) vec3 diffuseLight;
	layout(offset = 64) vec3 specularLight;
	layout(offset = 80) float shininess;
} lighting;

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

    vec3 posInCameraSpace = (mv * vec4(inPosition, 1.0)).xyz;
    vec3 lightPosInCameraSpace = (dynamicVP.view * vec4(lighting.lightPosInWorldSpace, 1.0)).xyz;
    vec3 lightDirInCameraSpace = normalize(lightPosInCameraSpace - posInCameraSpace);

    vec3 viewDirInCameraSpace = normalize(-posInCameraSpace);

    vec3 tangentInCameraSpace = normalize((mv * vec4(inTangent, 1.0)).xyz);
    vec3 bitangentInCameraSpace = normalize((mv * vec4(inBitangent, 1.0)).xyz);
    vec3 normalInCameraSpace = normalize((transpose(inverse(mv)) * vec4(inNormal, 0.0)).xyz);
    mat3 toTangentSpace = transpose(mat3(tangentInCameraSpace, bitangentInCameraSpace, normalInCameraSpace));

    vs_out.lightDirInTangentSpace = normalize(toTangentSpace * lightDirInCameraSpace);
    vs_out.viewDirInTangentSpace = normalize(toTangentSpace * viewDirInCameraSpace);
    vs_out.texCoords = inTexCoord;
}