#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding=0,std140) uniform Color {
	uint color;
};

layout(location=0) out vec4 fragColor;


void main() {

	fragColor=unpackUnorm4x8(color);

}
