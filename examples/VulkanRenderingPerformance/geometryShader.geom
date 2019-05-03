#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(triangles) in;
layout(triangle_strip,max_vertices=3) out;

layout(location=0) flat in int vertexIndex[];

layout(std430,binding=0) restrict readonly buffer PackedDataBuffer1 {
	uvec4 packedData1[];  // 0: float posX, 1: float posY, 2: float posZ, 3: half normalZ + half posW
};
layout(std430,binding=1) restrict readonly buffer PackedDataBuffer2 {
	uvec4 packedData2[];  // 0: float texU, 1: float texV, 2: half normalX + half normalY, 3: uint color
};
layout(std430,binding=2) restrict readonly buffer TransformationMatrix {
	mat4 transformationMatrix[];
};

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	mat4 matrix=transformationMatrix[vertexIndex[0]/3];
	for(uint i=0; i<3; i++) {
		uvec4 data1=packedData1[vertexIndex[i]];
		uvec4 data2=packedData2[vertexIndex[i]];
		vec2 extra=unpackHalf2x16(data1.w);
		vec4 position=vec4(uintBitsToFloat(data1.xyz),extra.y);
		vec3 normal=vec3(unpackHalf2x16(data2.z),extra.x);
		vec4 color=unpackUnorm4x8(data2.w);
		vec2 texCoord=data2.xy;
		gl_Position=matrix*position*color*vec4(normal,1)*vec4(texCoord,1,1);
		EmitVertex();
	}
}
