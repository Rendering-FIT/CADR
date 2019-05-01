#version 450
#extension GL_ARB_separate_shader_objects : enable

struct PackedData1 {
	vec3 position;
	uint extra1;  // half normalZ + half posW
};
struct PackedData2 {
	vec2 texCoord;
	uint extra2;  // half normalX + half normalY
	uint packedColor;
};

layout(std430,binding=0) restrict readonly buffer PackedDataBuffer1 {
	PackedData1 packedData1[];
};
layout(std430,binding=1) restrict readonly buffer PackedDataBuffer2 {
	PackedData2 packedData2[];
};

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	vec2 normalZandPosW=unpackHalf2x16(packedData1[gl_VertexIndex].extra1);
	vec3 normal=vec3(unpackHalf2x16(packedData2[gl_VertexIndex].extra2),normalZandPosW.x);
	gl_Position=vec4(packedData1[gl_VertexIndex].position,normalZandPosW.y)*
	            unpackUnorm4x8(packedData2[gl_VertexIndex].packedColor)*
	            vec4(unpackHalf2x16(packedData2[gl_VertexIndex].extra2),normalZandPosW.x,1)*
	            vec4(packedData2[gl_VertexIndex].texCoord,1,1);
}
