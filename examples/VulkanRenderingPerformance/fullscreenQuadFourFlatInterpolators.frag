#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in flat vec4 v1;
layout(location=1) in flat vec4 v2;
layout(location=2) in flat vec4 v3;
layout(location=3) in flat vec4 v4;

layout(location=0) out vec4 fragColor;


void main() {
	fragColor=vec4(dot(v1,v3),dot(v2,v4),1.,1.);
}
