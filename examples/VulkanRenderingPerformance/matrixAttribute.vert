#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec4 inPosition;
layout(location=1) in mat4 transformationMatrix;

out gl_PerVertex {
	vec4 gl_Position;
};


void main() {
	gl_Position=transformationMatrix*inPosition;
}
