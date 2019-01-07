#version 440

#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 projection;
} mvp;
layout(binding = 3) uniform Lighting {
    vec3 worldLightPos;
} lighting;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;
layout(location = 5) in vec3 inColor;

layout(location = 0) out vec3 worldPos;
//layout(location = 1) out vec3 cameraViewDir;
//layout(location = 2) out vec3 cameraLightDir;
layout(location = 3) out vec3 tangentLightDir;
//layout(location = 4) out vec3 tangentViewDir;
layout(location = 5) out vec2 fragTexCoord;

void main()
{
    mat4 mv = mvp.view * mvp.model;
    gl_Position = mvp.projection * mvp.view * mvp.model * vec4(inPosition, 1.0);

    worldPos = (mvp.model * vec4(inPosition, 1.0)).xyz;

    vec3 cameraPos = (mv * vec4(inPosition, 1.0)).xyz;
    //cameraViewDir = normalize(-cameraPos); // camera space origin - camera space vertex position

    vec3 cameraLightPos = (mvp.view * vec4(lighting.worldLightPos, 1)).xyz;
    vec3 cameraLightDir = normalize(cameraLightPos - cameraPos); // camera space light position - camera space vertex position

    mat3 mv3 = mat3(mv);
    vec3 cameraTangent = (mv * normalize(vec4(inTangent, 1.0))).xyz;
    vec3 cameraBitangent = (mv * normalize(vec4(inBitangent, 1.0))).xyz;
    vec3 cameraNormal = normalize((transpose(inverse(mv)) * vec4(inNormal, 1.0)).xyz);
    mat3 toTangentSpace = transpose(mat3(cameraTangent, cameraBitangent, cameraNormal));

    tangentLightDir = toTangentSpace * cameraLightDir;
    //tangentViewDir = toTangentSpace * cameraViewDir;

    fragTexCoord = inTexCoord;
}