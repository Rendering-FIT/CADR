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
layout(location=0) out vec4 eyePosition;
layout(location=1) out vec3 eyeNormal;
layout(location=2) out vec4 color;
layout(location=3) out vec2 texCoord;


void main() {
	gl_Position=vec4(vertices2D[gl_VertexIndex],0.6-(gl_InstanceIndex*0.05),1.);  // note: make sure that instances fit into the clip space
	eyePosition=gl_Position;
	eyeNormal=-gl_Position.xyz;
	color=max(gl_Position,vec4(0.5,0.5,0.5,0.5));
	texCoord=-gl_Position.yz;
}
