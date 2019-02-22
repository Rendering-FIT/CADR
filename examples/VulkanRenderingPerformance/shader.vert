#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec4 inPosition;

out gl_PerVertex {
	vec4 gl_Position;
};


void main() {
	gl_Position=inPosition;
}
