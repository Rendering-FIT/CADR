// SPDX-FileCopyrightText: 2022-2026 PCJohn (Jan Pečiva, peciva@fit.vut.cz)
//
// SPDX-License-Identifier: MIT-0

#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : require


//
//  push constants
//

layout(push_constant) uniform
pushConstants {
	layout(offset=8) uint64_t drawablePointersBufferPtr;  // pointer to DrawablePointersRef array; there is one DrawablePointersRef array for each StateSet, so the pointer is updated before each StateSet rendering
};


//
//  drawable data
//

layout(buffer_reference, std430, buffer_reference_align=64) restrict readonly buffer
MatrixListRef {
	uint numMatrices;
	uint capacity;
	uint64_t padding[7];
	mat4 matrices[];
};

// indices
layout(buffer_reference, std430, buffer_reference_align=4) restrict readonly buffer
IndexDataRef {
	uint indices[];
};

// drawable data pointers
layout(buffer_reference, std430, buffer_reference_align=8) restrict readonly buffer
DrawablePointersRef {
	uint64_t vertexDataPtr;
	uint64_t indexDataPtr;
	uint64_t matrixListPtr;
	uint64_t drawableDataPtr;
};
const uint DrawablePointersSize = 32;


//
//  vertex data
//

layout(buffer_reference, std430, buffer_reference_align=16) restrict readonly buffer
VertexDataRef {
	vec3 position;
	uint color;
};
const uint vertexDataSize = 16;


//
//  output
//

out gl_PerVertex {
	vec4 gl_Position;
};

layout(location = 0) out vec4 outColor;


//
//  main
//

void main()
{
	// DrawablePointers
	DrawablePointersRef dp = DrawablePointersRef(drawablePointersBufferPtr + (gl_DrawID * DrawablePointersSize));

	// model matrix list
	MatrixListRef modelMatrixList = MatrixListRef(dp.matrixListPtr);

	// vertex data
	IndexDataRef indexData = IndexDataRef(dp.indexDataPtr);
	uint index = indexData.indices[gl_VertexIndex];
	uint64_t vertexDataPtr = dp.vertexDataPtr + (index * vertexDataSize);
	VertexDataRef vertexData = VertexDataRef(vertexDataPtr);

	// output
	gl_Position = modelMatrixList.matrices[gl_InstanceIndex] * vec4(vertexData.position, 1.0);
	outColor = unpackUnorm4x8(vertexData.color);
}
