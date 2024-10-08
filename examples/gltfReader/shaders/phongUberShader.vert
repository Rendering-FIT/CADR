#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : require
#if 0  // debug print; this makes possible to use debugPrintfEXT() in this shader;
       // for more info, see comments around debugPrintEXT() bellow
# extension GL_EXT_debug_printf : require
#endif


// vertex
layout(buffer_reference, std430, buffer_reference_align=16) restrict readonly buffer VertexDataRef {
	vec3 position;
	float dummy1;
	vec3 normal;
	float dummy2;
#ifdef PER_VERTEX_COLOR
	vec4 color;
#endif
#ifdef TEXTURING
	vec2 texCoord;
	vec2 dummy3;
#endif
};
#ifdef PER_VERTEX_COLOR
# ifdef TEXTURING
const uint VertexDataSize = 64;
# else
const uint VertexDataSize = 48;
# endif
#else
# ifdef TEXTURING
const uint VertexDataSize = 48;
# else
const uint VertexDataSize = 32;
# endif
#endif

layout(buffer_reference, std430, buffer_reference_align=4) restrict readonly buffer IndexDataRef {
	uint indices[];
};

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

// payload data
// holding per-drawable data pointers
layout(buffer_reference, std430, buffer_reference_align=8) restrict readonly buffer PayloadDataRef {
	uint64_t vertexDataPtr;
	uint64_t indexDataPtr;
	uint64_t shaderDataPtr;
};
const uint PayloadDataSize = 24;


// Phong light source
struct Light {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec4 eyePosition;
	vec3 eyePositionDir;
};

// scene data
layout(buffer_reference, std430, buffer_reference_align=64) restrict readonly buffer SceneDataRef {
	mat4 viewMatrix;        // current camera view matrix
	float p11,p22,p33,p43;  // projectionMatrix - members that depend on zNear and zFar clipping planes
	vec3 ambientLight;
	uint numLights;
	Light lights[];
};


// push constants
layout(push_constant) uniform pushConstants {
	uint64_t payloadBufferPtr;  // one buffer for the whole scene
	uint64_t sceneDataPtr;  // one buffer for the whole scene
};

// projection matrix specialization constants
// (projectionMatrix members that do not depend on zNear and zFar clipping planes)
layout(constant_id = 0) const float p31 = 0.;
layout(constant_id = 1) const float p32 = 0.;
layout(constant_id = 2) const float p34 = 0.;
layout(constant_id = 3) const float p41 = 0.;
layout(constant_id = 4) const float p42 = 0.;
layout(constant_id = 5) const float p44 = 1.;


// output variables
out gl_PerVertex {
	vec4 gl_Position;
#ifdef POINT_SHADER
	float gl_PointSize;
#endif
};
layout(location = 0) flat out uint64_t outDataPtr;
layout(location = 1) smooth out vec3 outEyePosition3;
layout(location = 2) smooth out vec3 outEyeNormal;
#ifdef PER_VERTEX_COLOR
layout(location = 3) smooth out vec4 outColor;
# ifdef TEXTURING
layout(location = 4) smooth out vec2 outTexCoord;
#  ifdef ID_BUFFER
layout(location = 5) flat out uvec2 outId;
#  endif
# else
#  ifdef ID_BUFFER
layout(location = 4) flat out uvec2 outId;
#  endif
# endif
#else
# ifdef TEXTURING
layout(location = 3) smooth out vec2 outTexCoord;
#  ifdef ID_BUFFER
layout(location = 4) flat out uvec2 outId;
#  endif
# else
#  ifdef ID_BUFFER
layout(location = 3) flat out uvec2 outId;
#  endif
# endif
#endif


void main()
{

	// memory pointers
	PayloadDataRef pd = PayloadDataRef(payloadBufferPtr + (gl_DrawID * PayloadDataSize));
	outDataPtr = pd.shaderDataPtr;
	ShaderDataRef data = ShaderDataRef(pd.shaderDataPtr);
	SceneDataRef scene = SceneDataRef(sceneDataPtr);
	IndexDataRef indexData = IndexDataRef(pd.indexDataPtr);
	uint index = indexData.indices[gl_VertexIndex];
	VertexDataRef vertex = VertexDataRef(pd.vertexDataPtr + (index * VertexDataSize));

	// matrices and positions
	mat4 modelViewMatrix = scene.viewMatrix * data.modelMatrix[gl_InstanceIndex];
	vec4 eyePosition = modelViewMatrix * vec4(vertex.position,1);
	outEyePosition3 = eyePosition.xyz / eyePosition.w;

	// multiplication by projection "matrix"
	gl_Position.x = scene.p11*eyePosition.x + p31*eyePosition.z + p41*eyePosition.w;
	gl_Position.y = scene.p22*eyePosition.y + p32*eyePosition.z + p42*eyePosition.w;
	gl_Position.z = scene.p33*eyePosition.z + scene.p43*eyePosition.w;
	gl_Position.w = p34*eyePosition.z + p44*eyePosition.w;

	// write outputs
#ifdef POINT_SHADER
	gl_PointSize = 1;
#endif
	outEyeNormal = normalize(mat3(modelViewMatrix) * vertex.normal);
#ifdef PER_VERTEX_COLOR
	outColor = vertex.color;
#endif
#ifdef TEXTURING
	outTexCoord = vertex.texCoord;
#endif
#ifdef ID_BUFFER
	outId[0] = gl_DrawID;
	outId[1] = gl_InstanceIndex;
#endif

#if 0  // debug print; to enable it, you also need to (1) enable GL_EXT_debug_printf extension on the beginning of this shader,
       // you need to (2) enable VK_KHR_shader_non_semantic_info device extension (look for VK_KHR_shader_non_semantic_info
       // text in InitAndFinalize.cpp) and you need to (3) have Vulkan Configurator (vkconfig) running and you need to
       // (4) enable "Debug Printf" in Vulkan Configurator in Shader-Based validation;
       //  you might also need to increase "Printf buffer size"
	debugPrintfEXT("Phong, DrawID: %u, Vertex: %u, Index: %u, Instance %u, dataPtr: %lx, VertexDataBase: 0x%lx, VertexDataPtr: 0x%lx, VertexDataSize: %u, position: %.2f,%.2f,%.2f.\n",
	               gl_DrawID, gl_VertexIndex, index, gl_InstanceIndex, outDataPtr, pd.vertexDataPtr, uint64_t(vertex), VertexDataSize, vertex.position.x, vertex.position.y, vertex.position.z);
#endif

}
