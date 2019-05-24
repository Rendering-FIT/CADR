#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(triangles) in;
layout(triangle_strip,max_vertices=3) out;

layout(location=0) in uvec4 packedData1[];  // 0-1: doubleX, 2-3: doubleY
layout(location=1) in uvec4 packedData2[];  // 0-1: doubleZ, 2: half normalX + half normalY, 3: half normalZ + half posW
layout(location=2) in uvec4 packedData3[];  // 0: float texU, 1: float texV, 2: uint color
layout(location=3) in int vertexIndex[];

layout(std430,binding=0) restrict readonly buffer ModelMatrix {
	dmat4 modelMatrix[];
};

layout(binding=1) uniform UniformBufferObject {
	dmat4 viewMatrix;
	mat4 projectionMatrix;
};

out gl_PerVertex {
	vec4 gl_Position;
};
layout(location=0) out vec4 eyePosition;
layout(location=1) out vec3 eyeNormal;
layout(location=2) out vec4 color;
layout(location=3) out vec2 texCoord;


void main() {

	dmat4 m=modelMatrix[vertexIndex[0]/3];
	for(uint i=0; i<3; i++) {

		// unpack data
		vec2 extra=unpackHalf2x16(packedData2[i].w);
		dvec4 position=dvec4(packDouble2x32(packedData1[i].xy),packDouble2x32(packedData1[i].zw),
		                     packDouble2x32(packedData2[i].xy),extra.y);
		vec3 normal=vec3(unpackHalf2x16(packedData2[i].z),extra.x);

		// compute outputs
		eyePosition=vec4(viewMatrix*m*position);
		gl_Position=projectionMatrix*eyePosition;
		eyeNormal=mat3(viewMatrix)*mat3(m)*normal;
		color=unpackUnorm4x8(packedData3[i].z);
		texCoord=uintBitsToFloat(packedData3[i].xy);
		EmitVertex();

	}
}
