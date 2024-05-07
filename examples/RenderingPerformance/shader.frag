#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : require


// per-drawable shader data
layout(buffer_reference, std430, buffer_reference_align=64) restrict readonly buffer ShaderDataRef {
	layout(offset= 0) vec4 ambientAndMaterialType;
	layout(offset=16) vec4 diffuseAndAlpha;
	layout(offset=32) vec3 specular;
	layout(offset=44) float shininess;
	layout(offset=48) vec3 emission;
	layout(offset=60) float pointSize;
	layout(offset=64) mat4 modelMatrix[];
};


// scene data
layout(buffer_reference, std430, buffer_reference_align=64) restrict readonly buffer SceneDataRef {
	mat4 viewMatrix;        // current camera view matrix
	float p11,p22,p33,p43;  // projectionMatrix - members that depend on zNear and zFar clipping planes
};


layout(push_constant) uniform pushConstants {
	layout(offset=8)  uint64_t sceneDataPtr;  // one buffer for the whole scene
	layout(offset=16) uint stateSetID;  // ID of the current StateSet
};


// input and output
layout(location = 0) flat in uint64_t inDataPtr;
layout(location = 1) smooth in vec3 inEyePosition3;
#ifdef ID_BUFFER
layout(location = 2) flat in uvec2 inId;
#endif

layout(location = 0) out vec4 outColor;
#ifdef ID_BUFFER
layout(location = 1) out uvec4 outId;
#endif


void main()
{
	ShaderDataRef data = ShaderDataRef(inDataPtr);
	SceneDataRef scene = SceneDataRef(sceneDataPtr);

	outColor.rgba = data.diffuseAndAlpha.rgba;
#ifdef ID_BUFFER
	outId[0] = stateSetID;
	outId[1] = inId[0];  // gl_DrawID - index of indirect drawing structure
	outId[2] = inId[1];  // gl_InstanceIndex
	outId[3] = gl_PrimitiveID;
#endif
}
