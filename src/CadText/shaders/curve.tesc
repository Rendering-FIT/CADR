#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : require

layout(vertices = 3) out;

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

layout(location = 0) flat in uint64_t inDataPtr[];

layout(location = 0) flat out uint64_t outDataPtr[];

in gl_PerVertex {
    vec4 gl_Position;
} gl_in[];

out gl_PerVertex {
    vec4 gl_Position;
} gl_out[];

bool isTriangleCCW(vec2 a, vec2 b, vec2 c) {
   float determinant = (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
   return determinant > 0;
}

void main() {
	// memory pointers
	ShaderDataRef shaderData = ShaderDataRef(inDataPtr[gl_InvocationID]);
	SceneDataRef sceneData = SceneDataRef(sceneDataPtr);
	outDataPtr[gl_InvocationID] = inDataPtr[gl_InvocationID];

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

	// Quadratic bezier curve control points in world space
	vec4 start = gl_in[0].gl_Position;
	vec4 control = gl_in[1].gl_Position;
	vec4 end = gl_in[2].gl_Position;

	// Quadratic bezier curve control points in clip space
	vec4 startClip = sceneData.projection * sceneData.view * start;
	vec4 controlClip = sceneData.projection * sceneData.view * control;
	vec4 endClip = sceneData.projection * sceneData.view * end;
	
	// Quadratic bezier curve control points in NDC
	vec3 startNDC = startClip.xyz / startClip.w;
	vec3 controlNDC = controlClip.xyz / controlClip.w;
	vec3 endNDC = endClip.xyz / endClip.w;

	vec2 viewport = vec2(sceneData.viewportWidth, sceneData.viewportHeight);
	
	// Quadratic bezier curve control points in screen space
	vec2 startScreen = (startNDC.xy + 1.f) / 2.f * viewport;
	vec2 controlScreen = (controlNDC.xy + 1.f) / 2.f * viewport;
	vec2 endScreen = (endNDC.xy + 1.f) / 2.f * viewport;

	float maxCurveLength = distance(startScreen, controlScreen) + distance(controlScreen, endScreen);
	float curvature = 2 * abs(cross(vec3(controlScreen - startScreen, 1.f), vec3(endScreen - startScreen, 1.f)).z) / (distance(startScreen, controlScreen) * distance(controlScreen, endScreen) * distance(endScreen, startScreen));

	float tessellationLevel = clamp(maxCurveLength - (1.f / curvature), 4.f, maxCurveLength);

	// The curve is on the part of the contour that defines a filled outline
	if(isTriangleCCW(start.xy, control.xy, end.xy)) {
		gl_TessLevelInner[0] = 1.f;
		gl_TessLevelOuter[0] = tessellationLevel / 2.f;
		gl_TessLevelOuter[1] = 1.f;
		gl_TessLevelOuter[2] = tessellationLevel / 2.f;
	}
	// The curve is on the part of the contour that defines a hole
	else {
		gl_TessLevelInner[0] = 1.f;
		gl_TessLevelOuter[0] = 1.f;
		gl_TessLevelOuter[1] = tessellationLevel;
		gl_TessLevelOuter[2] = 1.f;
	}
}
