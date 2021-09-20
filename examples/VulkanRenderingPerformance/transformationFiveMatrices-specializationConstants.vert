#version 450

layout(location=0) in uvec4 packedData1;  // 0: float posX, 1: float posY, 2: float posZ, 3: half normalZ + half posW
layout(location=1) in uvec4 packedData2;  // 0: float texU, 1: float texV, 2: half normalX + half normalY, 3: uint color

layout(std430,binding=0) restrict readonly buffer TransformationMatrix {
	mat4 transformationMatrix[];
};

layout(std430,binding=1) restrict readonly buffer NormalMatrix {
	mat3 normalMatrix[];
};

layout(constant_id =  0) const float p11 = 1.;
layout(constant_id =  1) const float p12 = 0.;
layout(constant_id =  2) const float p13 = 0.;
layout(constant_id =  3) const float p14 = 0.;
layout(constant_id =  4) const float p21 = 0.;
layout(constant_id =  5) const float p22 = 1.;
layout(constant_id =  6) const float p23 = 0.;
layout(constant_id =  7) const float p24 = 0.;
layout(constant_id =  8) const float p31 = 0.;
layout(constant_id =  9) const float p32 = 0.;
layout(constant_id = 10) const float p33 = 1.;
layout(constant_id = 11) const float p34 = 0.;
layout(constant_id = 12) const float p41 = 0.;
layout(constant_id = 13) const float p42 = 0.;
layout(constant_id = 14) const float p43 = 0.;
layout(constant_id = 15) const float p44 = 1.;

layout(constant_id = 16) const float v11 = 1.;
layout(constant_id = 17) const float v12 = 0.;
layout(constant_id = 18) const float v13 = 0.;
layout(constant_id = 19) const float v14 = 0.;
layout(constant_id = 20) const float v21 = 0.;
layout(constant_id = 21) const float v22 = 1.;
layout(constant_id = 22) const float v23 = 0.;
layout(constant_id = 23) const float v24 = 0.;
layout(constant_id = 24) const float v31 = 0.;
layout(constant_id = 25) const float v32 = 0.;
layout(constant_id = 26) const float v33 = 1.;
layout(constant_id = 27) const float v34 = 0.;
layout(constant_id = 28) const float v41 = 0.;
layout(constant_id = 29) const float v42 = 0.;
layout(constant_id = 30) const float v43 = 0.;
layout(constant_id = 31) const float v44 = 1.;

const mat3 normalViewMatrix = mat3(1.);

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	vec2 extra = unpackHalf2x16(packedData1.w);
	vec4 position = vec4(uintBitsToFloat(packedData1.xyz), extra.y);
	vec3 normal = vec3(unpackHalf2x16(packedData2.z), extra.x); 
	vec4 color = unpackUnorm4x8(packedData2.w);
	vec2 texCoord = uintBitsToFloat(packedData2.xy);
	gl_Position = mat4(p11,p12,p13,p14, p21,p22,p23,p24, p31,p32,p33,p34, p41,p42,p43,p44) *
	              mat4(v11,v12,v13,v14, v21,v22,v23,v24, v31,v32,v33,v34, v41,v42,v43,v44) *
	              transformationMatrix[gl_VertexIndex/3] * position * color *
	              vec4(normalViewMatrix*normalMatrix[gl_VertexIndex/3]*normal, 1) * vec4(texCoord, 1, 1);
}
