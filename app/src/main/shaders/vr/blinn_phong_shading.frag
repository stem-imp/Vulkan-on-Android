#version 440

layout(location = 0) in VS_OUT {
    vec3 lightDirInTangentSpace;
    vec3 viewDirInTangentSpace;
    vec2 texCoords;
} fs_in;

layout(binding = 2) uniform sampler2D texSampler;
layout(binding = 3) uniform sampler2D normalSampler;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 halfDir = normalize(fs_in.lightDirInTangentSpace + fs_in.viewDirInTangentSpace);

    vec3 normalInTangentSpace = normalize(texture(normalSampler, fs_in.texCoords).rgb * 2.0 - 1.0);
    float cos = max(dot(fs_in.lightDirInTangentSpace, normalInTangentSpace), 0.0);
    // ambient light: vec3(0.125)
    vec3 ambient = cos * vec3(0.125);
    // diffuse light: vec3(0.875);
    vec3 diffuse = cos * vec3(0.875);
    // shiness: 32
    float specular = pow(max(dot(normalInTangentSpace, halfDir), 0.0), 32.0);
    // specular light: vec3(0.5)
    outColor = vec4((ambient + diffuse) * texture(texSampler, fs_in.texCoords).rgb + specular * vec3(0.5), 1.0);
}