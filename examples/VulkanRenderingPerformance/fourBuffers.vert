#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(std430,binding=0) restrict readonly buffer CoordinateBuffer {
	vec4 coordinateBuffer[];
};
layout(std430,binding=1) restrict readonly buffer Data1Buffer {
	vec4 data1[]; // contains vec4(-2,-2,-2,-2)
};
layout(std430,binding=2) restrict readonly buffer Data2Buffer {
	vec4 data2[]; // contains vec4(-2,-2,-2,-2)
};
layout(std430,binding=3) restrict readonly buffer Data3Buffer {
	vec4 data3[]; // contains vec4(4,4,4,4)
};

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = coordinateBuffer[gl_VertexIndex] + data1[gl_VertexIndex] +
	              data2[gl_VertexIndex] + data3[gl_VertexIndex];
}
