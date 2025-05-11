#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : require

layout(triangles, equal_spacing, ccw) in;

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

layout(location = 0) flat out uint64_t outDataPtr;

in gl_PerVertex {
    vec4 gl_Position;
} gl_in[];

// output variables
out gl_PerVertex {
	vec4 gl_Position;
};

bool isTriangleCCW(vec2 a, vec2 b, vec2 c) {
   float determinant = (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
   return determinant > 0;
}

// Evaluate quadratic bezier curve
vec2 bezier(float t, vec2 start, vec2 control, vec2 end) {
	return (1.f - t) * (1.f - t) * start + 2 * t * (1.f - t) * control + t * t * end;
}

void main() {
	// memory pointers
	ShaderDataRef shaderData = ShaderDataRef(inDataPtr[0]);
	SceneDataRef sceneData = SceneDataRef(sceneDataPtr);
	outDataPtr = inDataPtr[0];

	// Barycentric coordinates
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;
	float w = gl_TessCoord.z;

	// Quadratic bezier curve control points
	vec2 start = gl_in[0].gl_Position.xy;
	vec2 control = gl_in[1].gl_Position.xy;
	vec2 end = gl_in[2].gl_Position.xy;

	vec2 position;
	if(isTriangleCCW(start, control, end)) {
		if(u == 0.f) {
			// Edge from curve control point to end point
			position = bezier(0.5 + (w / 2.f), start, control, end);
		}
		else if (w == 0.f) {
			// Edge from curve start point to control point
			position = bezier(v / 2.f, start, control, end);
		}
		else {
			position = u * start + v * control + w * end;
		}
	}
	else {
		if(v == 0.f) {
			position = bezier(w, start, control, end);
		}
		else {
			position = control;
		}
	}

	gl_Position = sceneData.projection * sceneData.view * vec4(position, 0.f, 1.f);
}
