#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec4 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec4 inColor;
layout(location=3) in vec2 inTexCoord;

layout(std430,binding=0) restrict readonly buffer ModelMatrix {
	mat4 modelMatrix[];
};

layout(binding=1) uniform UniformBufferObject {
	mat4 viewMatrix;
	mat4 projectionMatrix;
};

out gl_PerVertex {
	vec4 gl_Position;
};
layout(location=0) out vec3 eyePosition;
layout(location=1) out vec3 eyeNormal;
layout(location=2) out vec4 outColor;
layout(location=3) out vec2 outTexCoord;


void main() {

	// compute outputs
	mat4 m=modelMatrix[gl_VertexIndex/3];
	vec4 eyePosition4=viewMatrix*m*inPosition;
	gl_Position=projectionMatrix*eyePosition4;
	eyePosition=eyePosition4.xyz;
	eyeNormal=mat3(viewMatrix)*mat3(m)*inNormal;
	outColor=inColor;
	outTexCoord=inTexCoord;

}
