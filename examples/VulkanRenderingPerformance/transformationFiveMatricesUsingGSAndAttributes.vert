#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in uvec4 packedData1in;  // 0: float posX, 1: float posY, 2: float posZ, 3: half normalZ + half posW
layout(location=1) in uvec4 packedData2in;  // 0: float texU, 1: float texV, 2: half normalX + half normalY, 3: uint color

layout(location=0) out uvec4 packedData1out;
layout(location=1) out uvec4 packedData2out;
layout(location=2) out int vertexIndex;

void main() {
	packedData1out=packedData1in;
	packedData2out=packedData2in;
	vertexIndex=gl_VertexIndex;
}
