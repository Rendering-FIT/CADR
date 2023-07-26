#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(triangles) in;
layout(triangle_strip,max_vertices=6) out;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position=vec4(0,0,0.5,1);
	EmitVertex();
	gl_Position=vec4(0,0,0.6,1);
	EmitVertex();
	gl_Position=vec4(0,1e-10,0.4,1);
	EmitVertex();
	EndPrimitive();
	gl_Position=vec4(0,0,0.7,1);
	EmitVertex();
	gl_Position=vec4(0,0,0.8,1);
	EmitVertex();
	gl_Position=vec4(0,1e-10,0.9,1);
	EmitVertex();
}
