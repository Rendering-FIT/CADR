#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding=0) uniform sampler2D t;

layout(location=0) out vec4 outColor;


void main() {
	outColor=texelFetch(t,ivec2(gl_FragCoord.x,gl_FragCoord.y),0);
}
