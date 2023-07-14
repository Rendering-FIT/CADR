#version 450
#extension GL_ARB_separate_shader_objects : enable

struct VertexData {
	vec4 pos;
	vec4 data1;  // contains vec4(-2,-2,-2,-2)
	vec4 data2;  // contains vec4(-2,-2,-2,-2)
	vec4 data3;  // contains vec4(4,4,4,4)
};

layout(std430, binding=0) restrict readonly buffer Buffer {
	VertexData vertexData[];
};

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	VertexData d = vertexData[gl_VertexIndex];
	gl_Position = d.pos + d.data1 + d.data2 + d.data3;
}
