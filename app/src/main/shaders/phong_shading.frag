#version 440

layout(location = 0) in vec3 worldPos;
//layout(location = 1) in vec3 cameraViewDir;
//layout(location = 2) in vec3 cameraLightDir;
layout(location = 3) in vec3 tangentLightDir;
//layout(location = 4) in vec3 tangentViewDir;
layout(location = 5) in vec2 fragTexCoord;

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D normalSampler;
//layout(binding = 3) uniform Lighting {
//    vec3 worldLightPos;
//} lighting;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 tangentNormal = normalize(texture(normalSampler, fragTexCoord).rgb * 2.0 - 1.0);

    //float distance = length(lighting.worldLightPos - worldPos);

    float cos = max(dot(tangentNormal, tangentLightDir), 0.0);
    vec3 diffuse = cos * vec3(1.0, 1.0, 1.0);

    outColor = vec4(diffuse * (texture(texSampler, fragTexCoord)).rgb, 1.0);
}