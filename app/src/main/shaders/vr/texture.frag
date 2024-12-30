#version 440

layout(location = 0) in VS_OUT {
    vec2 texCoords;
} fs_in;

layout(binding = 2) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(texture(texSampler, fs_in.texCoords).rgb, 1.0);
}