#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : require
#if 0  // debug print; this makes possible to use debugPrintfEXT() in this shader;
       // for more info, see comments around debugPrintEXT() bellow
# extension GL_EXT_debug_printf : require
#endif


// vertices
layout(buffer_reference, std430, buffer_reference_align=4) restrict readonly buffer
VertexDataRef {
	vec3 position;
};
const uint VertexDataSize = 12;


// indices
layout(buffer_reference, std430, buffer_reference_align=4) restrict readonly buffer
IndexDataRef {
	uint indices[];
};


// matrix list
layout(buffer_reference, std430, buffer_reference_align=64) restrict readonly buffer
MatrixListRef {
	uint numMatrices;
	uint capacity;
	uint64_t padding[7];
	mat4 matrices[];
};


// per-drawable data
layout(buffer_reference, std430, buffer_reference_align=64) restrict readonly buffer
DrawableDataRef {
	layout(offset= 0) vec4 ambientAndMaterialType;
	layout(offset=16) vec4 diffuseAndAlpha;
	layout(offset=32) vec3 specular;
	layout(offset=44) float shininess;
	layout(offset=48) vec3 emission;
	layout(offset=60) float pointSize;
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


// scene data
layout(buffer_reference, std430, buffer_reference_align=64) restrict readonly buffer
SceneDataRef {
	mat4 viewMatrix;        // current camera view matrix
	float p11,p22,p33,p43;  // projectionMatrix - members that depend on zNear and zFar clipping planes
};


// push constants
layout(push_constant) uniform
pushConstants {
	layout(offset=0) uint64_t sceneDataPtr;  // pointer to SceneDataRef; usually updated per scene render pass or once per scene rendering
	layout(offset=8) uint64_t drawablePointersBufferPtr;  // pointer to DrawablePointersRef array; there is one DrawablePointersRef array for each StateSet, so the pointer is updated before each StateSet rendering
};


// projection matrix specialization constants
// (projectionMatrix members that do not depend on zNear and zFar clipping planes)
layout(constant_id = 0) const float p31 = 0.;
layout(constant_id = 1) const float p32 = 0.;
layout(constant_id = 2) const float p34 = 0.;
layout(constant_id = 3) const float p41 = 0.;
layout(constant_id = 4) const float p42 = 0.;
layout(constant_id = 5) const float p44 = 0.;


// output variables
out gl_PerVertex {
	vec4 gl_Position;
};
layout(location = 0) flat out uint64_t outDrawableDataPtr;
layout(location = 1) smooth out vec3 outEyePosition3;
#ifdef ID_BUFFER
layout(location = 2) flat out uvec2 outId;
#endif



void main()
{
	// drawable pointers
	DrawablePointersRef dp = DrawablePointersRef(drawablePointersBufferPtr + (gl_DrawID * DrawablePointersSize));
	outDrawableDataPtr = dp.drawableDataPtr;

	// model matrix list
	MatrixListRef modelMatrixList = MatrixListRef(dp.matrixListPtr);

	// vertex data
	IndexDataRef indexData = IndexDataRef(dp.indexDataPtr);
	uint index = indexData.indices[gl_VertexIndex];
	VertexDataRef vertex = VertexDataRef(dp.vertexDataPtr + (index * VertexDataSize));

	// matrices and positions
	SceneDataRef scene = SceneDataRef(sceneDataPtr);
	mat4 modelViewMatrix = scene.viewMatrix * modelMatrixList.matrices[gl_InstanceIndex];
	vec4 eyePosition = modelViewMatrix * vec4(vertex.position, 1);
	outEyePosition3 = eyePosition.xyz / eyePosition.w;

	// multiplication by projection "matrix"
	gl_Position.x = scene.p11*eyePosition.x + p31*eyePosition.z + p41*eyePosition.w;
	gl_Position.y = scene.p22*eyePosition.y + p32*eyePosition.z + p42*eyePosition.w;
	gl_Position.z = scene.p33*eyePosition.z + scene.p43*eyePosition.w;
	gl_Position.w = p34*eyePosition.z + p44*eyePosition.w;

#ifdef ID_BUFFER
	outId[0] = gl_DrawID;
	outId[1] = gl_InstanceIndex;
#endif

#if 0  // debug print; to enable it, you also need to (1) enable GL_EXT_debug_printf extension on the beginning of this shader,
       // you need to (2) enable VK_KHR_shader_non_semantic_info device extension (look for VK_KHR_shader_non_semantic_info
       // text in InitAndFinalize.cpp) and you need to (3) have Vulkan Configurator (vkconfig) running and you need to
       // (4) enable "Debug Printf" in Vulkan Configurator in Shader-Based validation;
       //  you might also need to increase "Printf buffer size"
	debugPrintfEXT("BaseColor, DrawID: %u, Vertex: %u, Index: %u, Instance %u, dataPtr: %lx, VertexDataBase: 0x%lx, VertexDataPtr: 0x%lx, VertexDataSize: %u, position: %.2f,%.2f,%.2f.\n",
	               gl_DrawID, gl_VertexIndex, index, gl_InstanceIndex, outDataPtr, pd.vertexDataPtr, uint64_t(vertex), VertexDataSize, vertex.position.x, vertex.position.y, vertex.position.z);
#endif
}
