// SPDX-FileCopyrightText: 2026 PCJohn (Jan Pečiva, peciva@fit.vut.cz)
//
// SPDX-License-Identifier: MIT-0

#version 460

const vec2 vertices2D[] = {
	{ -2.,-2. },
	{ -2.,+6. },
	{ +6.,-2. },
};


void main()
{
	gl_Position = vec4(vertices2D[gl_VertexIndex], 0., 1.);
}
