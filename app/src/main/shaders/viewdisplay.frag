#version 440

#extension GL_ARB_separate_shader_objects : enable

layout (binding = 3) uniform sampler2D samplerView;

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outColor;

void main()
{
    vec2 uv = (inUV.xy + vec2(1.0, 1.0)) / 2.0;
	outColor = vec4((texture(samplerView, uv)).rgb, 1.0);
}