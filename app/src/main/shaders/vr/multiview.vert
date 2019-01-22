#version 440

layout(location = 0) in vec3 inPosition;

layout (location = 0) out vec2 outUV;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
    outUV = inPosition.xy;
	float K1 = -0.046875;
	//float K2 = 0;
	float r2 = dot(inPosition.xy, inPosition.xy);
	//float r4 = r2 * r2;
	gl_Position = vec4(inPosition.xy * (1 + K1 * r2/* + K2 * r4*/), inPosition.z, 1.0);
}