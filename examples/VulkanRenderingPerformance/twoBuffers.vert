#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(std430,binding=0) restrict readonly buffer CoordinateBuffer {
	vec4 coordinateBuffer[];
};
layout(std430,binding=1) restrict readonly buffer Data1Buffer {
	vec4 data1[]; // contains vec4(-2,-2,-2,-2)
};

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = coordinateBuffer[gl_VertexIndex] + data1[gl_VertexIndex] + vec4(2.,2.,2.,2.);
}
