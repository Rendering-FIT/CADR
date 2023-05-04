#version 450

layout(location = 0) out vec4 outColor;
layout(location = 1) out uvec4 outId;


void main()
{
	outColor = vec4(1, 1, 1, 1);
	outId = uvec4(0, 0, 0, 0);
}
