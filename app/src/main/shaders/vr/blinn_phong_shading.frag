#version 440

layout(location = 0) in VS_OUT {
    vec3 lightDirInTangentSpace;
    vec3 viewDirInTangentSpace;
    vec2 texCoords;
} fs_in;

layout(binding = 2) uniform sampler2D texSampler;
layout(binding = 3) uniform sampler2D normalSampler;

layout(push_constant) uniform Lighting {
	vec3 cameraPosInWorldSpace;
	layout(offset = 16) vec3 lightPosInWorldSpace;
	layout(offset = 32) vec3 ambientLight;
	layout(offset = 48) vec3 diffuseLight;
	layout(offset = 64) vec3 specularLight;
	layout(offset = 80) float shininess;
} lighting;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 lightDirInTangentSpace = normalize(fs_in.lightDirInTangentSpace);
    vec3 halfDir = normalize(lightDirInTangentSpace + normalize(fs_in.viewDirInTangentSpace));

    vec3 normalInTangentSpace = normalize(texture(normalSampler, fs_in.texCoords).rgb * 2.0 - 1.0);
    float cos = max(dot(lightDirInTangentSpace, normalInTangentSpace), 0.0);
    vec3 ambient = lighting.ambientLight;
    vec3 diffuse = cos * lighting.diffuseLight;
    float specular = pow(max(dot(normalInTangentSpace, halfDir), 0.0), lighting.shininess);
    outColor = vec4((ambient + diffuse ) * texture(texSampler, fs_in.texCoords).rgb + specular * lighting.specularLight, 1.0);
}