#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) flat out int vertexIndex;

void main() {
	vertexIndex=gl_VertexIndex;
}
