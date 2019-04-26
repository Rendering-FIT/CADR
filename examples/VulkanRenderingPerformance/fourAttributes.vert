#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec4 inPosition;
layout(location=1) in vec4 attribute1; // contains vec4(-2,-2,-2,-2)
layout(location=2) in vec4 attribute2; // contains vec4(-2,-2,-2,-2) or vec4(1,1,1,1)
layout(location=3) in vec4 attribute3; // contains vec4(4,4,4,4) or vec4(1,1,1,1)

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position=inPosition+attribute1+attribute2+attribute3;
}
