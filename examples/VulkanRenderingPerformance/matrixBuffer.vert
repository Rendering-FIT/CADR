#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec4 inPosition;

out gl_PerVertex {
	vec4 gl_Position;
};

layout(std430,binding=0) restrict readonly buffer TransformationMatrix {
	mat4 transformationMatrix[];
};


void main() {
	gl_Position=transformationMatrix[gl_VertexIndex/3]*inPosition;
}
