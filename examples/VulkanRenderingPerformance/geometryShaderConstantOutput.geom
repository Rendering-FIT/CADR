#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(triangles) in;
layout(triangle_strip,max_vertices=3) out;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position=vec4(0,2,0.5,1);
	EmitVertex();
	gl_Position=vec4(1,2,0.5,1);
	EmitVertex();
	gl_Position=vec4(1,3,0.5,1);
	EmitVertex();
}
