#version 450

layout(push_constant) uniform pushConstants {
	mat4 projectionMatrix;
	mat4 viewMatrix;
};

layout(location = 0) in vec3 inPosition;

out gl_PerVertex {
	vec4 gl_Position;
};


void main()
{
	gl_Position = projectionMatrix * viewMatrix * vec4(inPosition.xyz, 1);
}
