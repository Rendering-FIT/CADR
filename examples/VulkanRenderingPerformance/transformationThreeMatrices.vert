#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in uvec4 packedData1;  // 0: float posX, 1: float posY, 2: float posZ, 3: half normalZ + half posW
layout(location=1) in uvec4 packedData2;  // 0: float texU, 1: float texV, 2: half normalX + half normalY, 3: uint color

layout(std430,binding=0) restrict readonly buffer TransformationMatrix {
	mat4 transformationMatrix[];
};

layout(binding=1) uniform UniformBufferObject {
	mat4 viewMatrix;
	mat4 projectionMatrix;
	mat3 normalViewMatrix;
};

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	vec2 extra=unpackHalf2x16(packedData1.w);
	vec4 position=vec4(uintBitsToFloat(packedData1.xyz),extra.y);
	vec3 normal=vec3(unpackHalf2x16(packedData2.z),extra.x);
	vec4 color=unpackUnorm4x8(packedData2.w);
	vec2 texCoord=uintBitsToFloat(packedData2.xy);
	gl_Position=projectionMatrix*viewMatrix*transformationMatrix[gl_VertexIndex/3]*position*color*vec4(normal,1)*vec4(texCoord,1,1);
}
