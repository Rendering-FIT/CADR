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


void main() {
	gl_Position=vec4(vertices2D[gl_VertexIndex],0.6-(gl_InstanceIndex*0.05),1.);  // note: make sure that instances fit into the clip space
}
