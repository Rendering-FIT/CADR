#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : require

// push constants
layout(push_constant) uniform pushConstants {
	uint64_t payloadBufferPtr; // one buffer for the whole scene
	uint64_t sceneDataPtr; // one buffer for the whole scene
};

// vertex buffer
layout(buffer_reference, std430, buffer_reference_align = 8) restrict readonly buffer VertexDataRef {
	vec2 position;
	vec2 texCoord;
};
const uint VertexDataSize = 16;

// index buffer
layout(buffer_reference, std430, buffer_reference_align = 4) restrict readonly buffer IndexDataRef {
	uint indices[];
};

// payload data holding per-drawable data pointers
layout(buffer_reference, std430, buffer_reference_align = 8) restrict readonly buffer PayloadDataRef {
	uint64_t vertexDataPtr;
	uint64_t indexDataPtr;
	uint64_t shaderDataPtr;
};
const uint PayloadDataSize = 24;

// per-drawable shader data
layout(buffer_reference, std430, buffer_reference_align = 64) restrict readonly buffer ShaderDataRef {
	layout(offset = 0) vec4 color;
	layout(offset = 16) mat4 model;
};
const uint ShaderDataSize = 80;

// scene data
layout(buffer_reference, std430, buffer_reference_align = 64) restrict readonly buffer SceneDataRef {
	uint viewportWidth;
	uint viewportHeight;
	vec2 padding1;
	mat4 view;
	mat4 projection;
};
const uint SceneDataSize = 144;

// output variables
out gl_PerVertex {
	vec4 gl_Position;
};

layout(location = 0) flat out uint64_t outDataPtr;
layout(location = 1) smooth out vec2 outTexCoord;

void main() {
	// memory pointers
	PayloadDataRef payloadData = PayloadDataRef(payloadBufferPtr + (gl_DrawID * PayloadDataSize));
	SceneDataRef sceneData = SceneDataRef(sceneDataPtr);
	ShaderDataRef shaderData = ShaderDataRef(payloadData.shaderDataPtr);
	IndexDataRef indexData = IndexDataRef(payloadData.indexDataPtr);
	uint index = indexData.indices[gl_VertexIndex];
	VertexDataRef vertex = VertexDataRef(payloadData.vertexDataPtr + (index * VertexDataSize));
	outDataPtr = payloadData.shaderDataPtr;

    gl_Position = sceneData.projection * sceneData.view * shaderData.model * vec4(vertex.position, 0.f, 1.f);
	outTexCoord = vertex.texCoord;
}
