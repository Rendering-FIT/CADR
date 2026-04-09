// SPDX-FileCopyrightText: 2018 PCJohn (Jan Pečiva, peciva@fit.vut.cz)
//
// SPDX-License-Identifier: MIT-0

#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
	vec4 gl_Position;
};


vec4 positions[4] = vec4[](
	vec4(-1.0,-1.0, 0.0, 1.0),
	vec4(-1.0, 1.0, 0.0, 1.0),
	vec4( 1.0,-1.0, 0.0, 1.0),
	vec4( 1.0, 1.0, 0.0, 1.0)
);


void main() {
	gl_Position=positions[gl_VertexIndex];
}
