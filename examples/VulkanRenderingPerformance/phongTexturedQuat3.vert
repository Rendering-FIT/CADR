#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in uvec4 packedData1;  // 0: float posX, 1: float posY, 2: float posZ, 3: half normalZ + half posW
layout(location=1) in uvec4 packedData2;  // 0: float texU, 1: float texV, 2: half normalX + half normalY, 3: uint color

layout(binding=0) restrict readonly buffer ModelMatrix {
	vec4 modelPAT[];  // model Position and Attitude Transformation, each transformation is composed of two vec4 - the first one is quaternion and the second translation
};

layout(binding=1) uniform UniformBufferObject {
	mat4x4 viewMatrix;
	mat4 projectionMatrix;
};

out gl_PerVertex {
	vec4 gl_Position;
};
layout(location=0) out vec3 eyePosition;
layout(location=1) out vec3 eyeNormal;
layout(location=2) out vec4 color;
layout(location=3) out vec2 texCoord;


void main() {

	// unpack data
	vec2 extra=unpackHalf2x16(packedData1.w);
	vec3 position=uintBitsToFloat(packedData1.xyz);
	vec3 normal=vec3(unpackHalf2x16(packedData2.z),extra.x);

	// prepare data
	uint i=gl_VertexIndex/3*2;
	vec4 q=modelPAT[i+0];
	vec3 t=modelPAT[i+1].xyz;
	float a=q.w*q.w-dot(q.xyz,q.xyz);
	vec4 b=2*q;

	// compute outputs
	vec3 worldPosition=(position*a)+(b.xyz*dot(q.xyz,position))+(b.w*cross(q.xyz,position))+t;
	eyePosition=mat3(viewMatrix)*worldPosition+viewMatrix[3].xyz;
	gl_Position=projectionMatrix*vec4(eyePosition,1);
	vec3 worldNormal=((normal*a)+(b.xyz*dot(q.xyz,normal))+(b.w*cross(q.xyz,normal)));
	eyeNormal=mat3(viewMatrix)*worldNormal;
	color=unpackUnorm4x8(packedData2.w);
	texCoord=uintBitsToFloat(packedData2.xy);

}
