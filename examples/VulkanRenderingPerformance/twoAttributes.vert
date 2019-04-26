#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec4 inPosition;
layout(location=1) in vec4 attribute1; // contains vec4(-2,-2,-2,-2)

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position=inPosition+attribute1+vec4(2.,2.,2.,2.);
}
