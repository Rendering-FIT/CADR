#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(std430,binding=0) restrict readonly buffer CoordinateBuffer {
	float coordinateBuffer[];
};
layout(std430,binding=1) restrict readonly buffer Data1Buffer {
	float data1[]; // contains vec4(-2,-2,-2,-2)
};
layout(std430,binding=2) restrict readonly buffer Data2Buffer {
	float data2[]; // contains vec4(-2,-2,-2,-2)
};
layout(std430,binding=3) restrict readonly buffer Data3Buffer {
	float data3[]; // contains vec4(4,4,4,4)
};

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	int index = gl_VertexIndex * 3;
	gl_Position.x = coordinateBuffer[index + 0] + data1[index + 0] +
	                data2[index + 0] + data3[index + 0];
	gl_Position.y = coordinateBuffer[index + 1] + data1[index + 1] +
	                data2[index + 1] + data3[index + 1];
	gl_Position.z = coordinateBuffer[index + 2] + data1[index + 2] +
	                data2[index + 2] + data3[index + 2];
	gl_Position.w = 1.;
}
