/*#version 440
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(binding = 2) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4((texture(texSampler, fragTexCoord)).rgb, 1.0);
    //outColor = vec4((vec4(fragColor.rgb, 1.0) * texture(texSampler, fragTexCoord)).rgb * 1.5f, 1.0);
    //outColor = vec4(fragColor.rgb, 1.0);
}*/