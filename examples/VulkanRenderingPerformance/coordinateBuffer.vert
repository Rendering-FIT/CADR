#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
	vec4 gl_Position;
};

layout(std430,binding=0) restrict readonly buffer CoordinateBuffer {
	vec4 coordinateBuffer[];
};


void main() {
	gl_Position=coordinateBuffer[gl_VertexIndex];
}
