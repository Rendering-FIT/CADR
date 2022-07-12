#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in smooth vec3 eyePosition;
layout(location=1) in smooth vec3 eyeNormal;
layout(location=2) in smooth vec4 color;
layout(location=3) in smooth vec2 texCoord;

layout(location=0) out vec4 fragColor;


void main() {
	fragColor=vec4(dot(vec4(eyePosition,1.),color),dot(eyeNormal,vec3(texCoord,1.)),0.,1.);
}
