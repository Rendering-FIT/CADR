#version 450
#extension GL_ARB_separate_shader_objects : enable

const vec2 vertices2D[] = {
	{ -1.,-1. },
	{ -1.,+1. },
	{ +1.,-1. },
	{ +1.,+1. },
};

out gl_PerVertex {
	vec4 gl_Position;
};
layout(location=0) out vec4 v1;
layout(location=1) out vec4 v2;
layout(location=2) out vec4 v3;
layout(location=3) out vec4 v4;


void main() {
	gl_Position=vec4(vertices2D[gl_VertexIndex],0.6-(gl_InstanceIndex*0.05),1.);  // note: make sure that instances fit into the clip space
	v1=gl_Position;
	v2=-gl_Position;
	v3=max(gl_Position,vec4(0.5,0.5,0.5,0.5));
	v4=-v3;
}
