#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding=0) uniform sampler2D colorTexture;
layout(binding=1) uniform sampler2D depthTexture;

layout(location=0) out vec4 outColor;


void main() {
	ivec2 uv=ivec2(gl_FragCoord.xy);
	outColor=texelFetch(colorTexture,uv,0);
	gl_FragDepth=texelFetch(depthTexture,uv,0).r;
}
