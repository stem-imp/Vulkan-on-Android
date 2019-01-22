#version 440

layout (binding = 0) uniform sampler2D resolvedView;

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outColor;

void main()
{
    vec2 uv = (inUV.xy + vec2(1.0, 1.0)) / 2.0;
	outColor = vec4((texture(resolvedView, uv)).rgb, 1.0);
}