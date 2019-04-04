#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec4 inPosition;

out gl_PerVertex {
	vec4 gl_Position;
};

layout(binding=0) uniform UniformBufferObject {
	mat4 modelView;
};


void main() {
	gl_Position=modelView*inPosition;
}
