#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
	vec4 gl_Position;
};

layout(location = 0) out vec3 fragColor;


// triangle coordinates
// given in Vulkan NDC (Normalized Device Coordinates; x,y ranging from -1 to +1, z ranging from 0 to +1;
// x points right, y points down, z points forward)
vec3 positions[6] = vec3[](
	vec3(-0.7, 0.3, 1.0),
	vec3(-0.2,-0.7, 0.5),
	vec3( 0.3, 0.3, 0.0),
	vec3(-0.3, 0.3, 0.0),
	vec3( 0.2,-0.7, 0.5),
	vec3( 0.7, 0.3, 1.0)
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
