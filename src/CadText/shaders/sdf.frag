#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : require

// push constants
layout(push_constant) uniform pushConstants {
	uint64_t payloadBufferPtr; // one buffer for the whole scene
	uint64_t sceneDataPtr; // one buffer for the whole scene
};

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

layout(set = 0, binding = 0) uniform sampler2D fontAtlas;

layout(location = 0) flat in uint64_t inDataPtr;
layout(location = 1) smooth in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    // memory pointers
	ShaderDataRef shaderData = ShaderDataRef(inDataPtr);
	SceneDataRef sceneData = SceneDataRef(sceneDataPtr);

	float distance = texture(fontAtlas, inTexCoord).r;
    float alpha = distance > 0.5 ? 1.f : 0.f;
    outColor = shaderData.color * vec4(1.f, 1.f, 1.f, alpha);

    // Prevent z-fighting
    if(alpha < 0.01) {
        discard;
    }
}
