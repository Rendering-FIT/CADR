#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec4 inPosition;
layout(location=1) in vec4 inColor;

out gl_PerVertex {
	vec4 gl_Position;
};
layout(location=0) out vec4 outColor;


void main() {
	gl_Position=inPosition;
	outColor=inColor;
}
