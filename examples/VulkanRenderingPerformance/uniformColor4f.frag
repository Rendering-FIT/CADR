#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding=0,std140) uniform Color {
	vec4 color;
};

layout(location=0) out vec4 fragColor;


void main() {

	fragColor=color;

}
