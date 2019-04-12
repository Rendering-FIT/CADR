#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec4 inPosition;
layout(location=1) in vec4 attribute1; // contains vec4(-2,-2,-2,-2)
layout(location=2) in vec4 attribute2; // contains vec4(-2,-2,-2,-2)
layout(location=3) in vec4 attribute3; // contains vec4(4,4,4,4)

out gl_PerVertex {
	vec4 gl_Position;
};

layout(std430,binding=0) restrict readonly buffer ModelViewMatrix {
	mat4 mvMatrix[];
};


void main() {
	gl_Position=mvMatrix[gl_VertexIndex/3]*(inPosition+attribute1+attribute2+attribute3);
}
