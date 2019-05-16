#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec4 eyePosition;
layout(location=1) in vec3 eyeNormal;
layout(location=2) in vec4 color;
layout(location=3) in vec2 texCoord;

layout(location=0) out vec4 fragColor;


void main() {
	fragColor=(color+vec4(eyeNormal,0.)-vec4(texCoord,0.,0.))*eyePosition;
}
