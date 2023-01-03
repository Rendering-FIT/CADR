#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
	vec4 gl_Position;
};

layout(binding=0) restrict readonly buffer CoordinateBuffer {
	float coordinateBuffer[];
};


void main() {
	int index = gl_VertexIndex * 3;
	gl_Position.x = coordinateBuffer[index + 0];
	gl_Position.y = coordinateBuffer[index + 1];
	gl_Position.z = coordinateBuffer[index + 2];
	gl_Position.w = 1.;
}
