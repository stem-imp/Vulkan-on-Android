#version 440

layout(location = 0) in VS_OUT {
    vec3 worldSpacePos;
    vec3 worldSpaceNormal;
    vec2 texCoords;
} fs_in;

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform Lighting {
    vec3 worldSpaceCameraPos;
    vec3 worldSpaceLightPos;
} lighting;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 lightDir = normalize(lighting.worldSpaceLightPos - fs_in.worldSpacePos);
    vec3 viewDir = normalize(lighting.worldSpaceCameraPos - fs_in.worldSpacePos);
    vec3 halfDir = normalize(lightDir + viewDir);
    vec3 normal = normalize(fs_in.worldSpaceNormal);
    float diff = max(dot(lightDir, normal), 0.0);
    // 32: shiness
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    // 0.125: ambient, vec3(0.5): specular light
    outColor = vec4(((0.125 + diff) * texture(texSampler, fs_in.texCoords)).rgb + spec * vec3(0.5), 1.0);
}