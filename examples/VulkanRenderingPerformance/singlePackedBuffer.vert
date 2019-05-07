#version 450
#extension GL_ARB_separate_shader_objects : enable

struct PackedData {
	vec3 position;
	uint packedColor;
	vec2 texCoord;
	uint extra1;  // half normalX + half normalY
	uint extra2;  // half normalZ + half posW
};

layout(std430,binding=0) restrict readonly buffer PackedDataBuffer {
	PackedData packedData[];
};

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	PackedData d=packedData[gl_VertexIndex];
	vec2 normalZandPosW=unpackHalf2x16(d.extra2);
	gl_Position=vec4(d.position,normalZandPosW.y)*
	            unpackUnorm4x8(d.packedColor)*
	            vec4(unpackHalf2x16(d.extra1),normalZandPosW.x,1)*
	            vec4(d.texCoord,1,1);
}
