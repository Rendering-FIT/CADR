#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
	vec4 gl_Position;
};

layout(location = 0) out vec3 fragColor;


vec3 positions[6] = vec3[](
	vec3(-0.7, 0.5, 0.4),
	vec3(-0.2,-0.5, 0.2),
	vec3( 0.3, 0.5, 0.0),
	vec3(-0.3, 0.5, 0.0),
	vec3( 0.2,-0.5, 0.2),
	vec3( 0.7, 0.5, 0.4)
);

vec3 colors[6] = vec3[](
	vec3(0.0,0.0,1.0),
	vec3(1.0,0.0,0.0),
	vec3(0.0,1.0,0.0),
	vec3(0.0,0.0,1.0),
	vec3(1.0,0.0,0.0),
	vec3(0.0,1.0,0.0)
);


void main() {
	gl_Position=vec4(positions[gl_VertexIndex],1.0);
	fragColor=colors[gl_VertexIndex];
}
