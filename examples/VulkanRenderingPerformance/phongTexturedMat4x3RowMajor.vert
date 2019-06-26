#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in uvec4 packedData1;  // 0: float posX, 1: float posY, 2: float posZ, 3: half normalZ + half posW
layout(location=1) in uvec4 packedData2;  // 0: float texU, 1: float texV, 2: half normalX + half normalY, 3: uint color

layout(binding=0) restrict readonly buffer ModelMatrix {
	layout(row_major) mat4x3 modelMatrix[];
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
layout(location=2) out vec4 color;
layout(location=3) out vec2 texCoord;


void main() {

	// unpack data
	vec2 extra=unpackHalf2x16(packedData1.w);
	vec3 position=uintBitsToFloat(packedData1.xyz);
	vec3 normal=vec3(unpackHalf2x16(packedData2.z),extra.x);

	// compute outputs
	mat4x3 m=modelMatrix[gl_VertexIndex/3];
	vec3 worldPosition=mat3(m)*position+m[3];
	eyePosition=mat3(viewMatrix)*worldPosition+viewMatrix[3].xyz;
	gl_Position=projectionMatrix*vec4(eyePosition,1);
	eyeNormal=mat3(viewMatrix)*mat3(m)*normal;
	color=unpackUnorm4x8(packedData2.w);
	texCoord=uintBitsToFloat(packedData2.xy);

}
