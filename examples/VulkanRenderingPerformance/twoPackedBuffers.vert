#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(std430,binding=0) restrict readonly buffer PackedDataBuffer1 {
	uvec4 packedData1[];  // 0: float posX, 1: float posY, 2: float posZ, 3: half normalZ + half posW
};
layout(std430,binding=1) restrict readonly buffer PackedDataBuffer2 {
	uvec4 packedData2[];  // 0: float texU, 1: float texV, 2: half normalX + half normalY, 3: uint color
};

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	uvec4 data1=packedData1[gl_VertexIndex];
	uvec4 data2=packedData2[gl_VertexIndex];
	vec2 extra=unpackHalf2x16(data1.w);
	vec4 position=vec4(uintBitsToFloat(data1.xyz),extra.y);
	vec3 normal=vec3(unpackHalf2x16(data2.z),extra.x);
	vec4 color=unpackUnorm4x8(data2.w);
	vec2 texCoord=data2.xy;
	gl_Position=position*color*vec4(normal,1)*vec4(texCoord,1,1);
}
