#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : require


// phong material data
layout(buffer_reference, std430, buffer_reference_align=16) restrict readonly buffer
PhongMaterialRef {
	layout(offset=0)  vec3 ambient;  //< ambient color might be ignored and replaced by diffuse color when specified by settings
	layout(offset=16) vec4 diffuseAndAlpha;  //< Hoops uses four floats for diffuse and alpha in MaterialKit
	layout(offset=32) vec3 specular;  //< Hoops uses specular color in MaterialKit
	layout(offset=44) float shininess;  //< Hoops uses gloss (1x float) in MaterialKit
	layout(offset=48) vec3 emission;  //< Hoops uses 4x float in MaterialKit
};


// scene data
layout(buffer_reference, std430, buffer_reference_align=64) restrict readonly buffer
SceneDataRef {
	mat4 viewMatrix;        // current camera view matrix
	float p11,p22,p33,p43;  // projectionMatrix - members that depend on zNear and zFar clipping planes
};


layout(push_constant) uniform
pushConstants {
	layout(offset=16) uint stateSetID;  // ID of the current StateSet
};


// input and output
layout(location = 0) flat in uint64_t inDrawableDataPtr;
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
	PhongMaterialRef drawableData = PhongMaterialRef(inDrawableDataPtr);
	outColor.rgba = drawableData.diffuseAndAlpha;
#ifdef ID_BUFFER
	outId[0] = stateSetID;
	outId[1] = inId[0];  // gl_DrawID - index of indirect drawing structure
	outId[2] = inId[1];  // gl_InstanceIndex
	outId[3] = gl_PrimitiveID;
#endif
}
