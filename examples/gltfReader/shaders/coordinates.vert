#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(push_constant) uniform pushConstants {
    mat4 projectionMatrix;
};

layout(location=0) in vec4 inPosition;

out gl_PerVertex {
	vec4 gl_Position;
};


void main() {
	gl_Position=projectionMatrix*vec4(inPosition.xy,3,1);
}
