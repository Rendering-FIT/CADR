#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in uvec4 packedData1in;  // 0-1: doubleX, 2-3: doubleY
layout(location=1) in uvec4 packedData2in;  // 0-1: doubleZ, 2: half normalX + half normalY, 3: half normalZ + half posW
layout(location=2) in uvec4 packedData3in;  // 0: float texU, 1: float texV, 2: uint color

layout(location=0) out uvec4 packedData1out;
layout(location=1) out uvec4 packedData2out;
layout(location=2) out uvec4 packedData3out;
layout(location=3) out int vertexIndex;

void main() {
	packedData1out=packedData1in;
	packedData2out=packedData2in;
	packedData3out=packedData3in;
	vertexIndex=gl_VertexIndex;
}
