#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : require
#extension GL_GOOGLE_include_directive : require
#include "UberShaderReadFuncs.glsl"
#include "UberShaderInterface.glsl"
#if 0  // debug print; this makes possible to use debugPrintfEXT() in this shader;
       // for more info, see comments around debugPrintEXT() bellow
# extension GL_EXT_debug_printf : require
#endif

#ifdef TRIANGLES
layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;
const int numInputs = 3;
#endif
#ifdef LINES
layout(lines) in;
layout(line_strip, max_vertices=2) out;
const int numInputs = 2;
#endif


// input from vertex shader
layout(location = 0) in flat int inDrawIndex[numInputs];
layout(location = 1) in flat int inInstanceIndex[numInputs];
layout(location = 2) in flat int inVertexIndex[numInputs];
#ifdef ID_BUFFER
layout(location = 3) in flat uvec2 inId[numInputs];
#endif

// output to fragment shader
layout(location = 0) out flat u64vec4 outVertexAndDrawableDataPtr;  // VertexData on indices 0..2 for triangles and 0..1 for lines. DrawableData on index 3. The vector occupies locations 0 and 1.
#ifdef TRIANGLES
layout(location = 2) out smooth vec3 outBarycentricCoords;  // barycentric coordinates using perspective correction
#endif
#ifdef LINES
layout(location = 2) out smooth vec2 outBarycentricCoords;  // barycentric coordinates using perspective correction
#endif
layout(location = 3) out smooth vec3 outVertexPosition3;  // in eye coordinates
layout(location = 4) out smooth vec3 outVertexNormal;  // in eye coordinates
layout(location = 5) out smooth vec3 outVertexTangent;  // in eye coordinates
#ifdef ID_BUFFER
layout(location = 6) out flat uvec2 outId;
#endif


// projection matrix specialization constants
// (projectionMatrix members that do not depend on zNear and zFar clipping planes)
layout(constant_id = 0) const float p31 = 0.;
layout(constant_id = 1) const float p32 = 0.;
layout(constant_id = 2) const float p34 = 0.;
layout(constant_id = 3) const float p41 = 0.;
layout(constant_id = 4) const float p42 = 0.;
layout(constant_id = 5) const float p44 = 1.;



void main()
{
	// input from vertex shader
	int drawIndex = inDrawIndex[0];
	int instanceIndex = inInstanceIndex[0];

	// DrawablePointers
	u64vec4 vertexAndDrawableDataPtr;
	DrawablePointersRef dp = DrawablePointersRef(drawablePointersBufferPtr + (drawIndex * DrawablePointersSize));
	vertexAndDrawableDataPtr.w = dp.drawableDataPtr;

	// model matrix list
	MatrixListRef modelMatrixList = MatrixListRef(dp.matrixListPtr);

	// vertex data
	IndexDataRef indexData = IndexDataRef(dp.indexDataPtr);
	uint vertexDataSize = getVertexDataSize();
	uint index0 = indexData.indices[inVertexIndex[0]];
	uint64_t vertex0DataPtr = dp.vertexDataPtr + (index0 * vertexDataSize);
	vertexAndDrawableDataPtr.x = vertex0DataPtr;
	uint index1 = indexData.indices[inVertexIndex[1]];
	uint64_t vertex1DataPtr = dp.vertexDataPtr + (index1 * vertexDataSize);
	vertexAndDrawableDataPtr.y = vertex1DataPtr;
#ifdef TRIANGLES
	uint index2 = indexData.indices[inVertexIndex[2]];
	uint64_t vertex2DataPtr = dp.vertexDataPtr + (index2 * vertexDataSize);
	vertexAndDrawableDataPtr.z = vertex2DataPtr;
#endif

	// vertex positions
	// (it is stored on offset 0 by convention)
	uint positionAccessInfo = getPositionAccessInfo();
	vec3 position0 = readVec3(vertex0DataPtr, positionAccessInfo);
	vec3 position1 = readVec3(vertex1DataPtr, positionAccessInfo);
#ifdef TRIANGLES
	vec3 position2 = readVec3(vertex2DataPtr, positionAccessInfo);
#endif

	// matrices and positions
	SceneDataRef scene = SceneDataRef(sceneDataPtr);
	mat4 modelViewMatrix = scene.viewMatrix * modelMatrixList.matrices[instanceIndex];
	vec4 eyePosition0 = modelViewMatrix * vec4(position0, 1);
	vec4 eyePosition1 = modelViewMatrix * vec4(position1, 1);
#ifdef TRIANGLES
	vec4 eyePosition2 = modelViewMatrix * vec4(position2, 1);
#endif
#if 0  // debug print; to enable it, you also need to (1) enable GL_EXT_debug_printf extension on the beginning of this shader,
       // you need to (2) enable VK_KHR_shader_non_semantic_info device extension (look for VK_KHR_shader_non_semantic_info
       // text in InitAndFinalize.cpp) and you need to (3) have Vulkan Configurator (vkconfig) running and you need to
       // (4) enable "Debug Printf" in Vulkan Configurator in Shader-Based validation;
       //  you might also need to increase "Printf buffer size"
	debugPrintfEXT("GS: %u,%u,%u, 0x%lx,0x%lx,0x%lx, %.2f,%.2f,%.2f, %.2f,%.2f,%.2f, %.2f,%.2f,%.2f.\n%.2f,%.2f,%.2f, %.2f,%.2f,%.2f, %.2f,%.2f,%.2f.\n",
	               index0, index1, index2, vertex0DataPtr, vertex1DataPtr, vertex2DataPtr,
	               position0.x, position0.y, position0.z,  position1.x, position1.y, position1.z,  position2.x, position2.y, position2.z,
	               eyePosition0.x, eyePosition0.y, eyePosition0.z,  eyePosition1.x, eyePosition1.y, eyePosition1.z,  eyePosition2.x, eyePosition2.y, eyePosition2.z);
#endif


	// first vertex

	// multiplication by projection "matrix"
#if 1
	gl_Position.x = scene.p11*eyePosition0.x + p31*eyePosition0.z + p41*eyePosition0.w;
	gl_Position.y = scene.p22*eyePosition0.y + p32*eyePosition0.z + p42*eyePosition0.w;
	gl_Position.z = scene.p33*eyePosition0.z + scene.p43*eyePosition0.w;
	gl_Position.w = p34*eyePosition0.z + p44*eyePosition0.w;
#else
	gl_Position = scene.projectionMatrix * eyePosition0;
#endif

	// set output variables
	outVertexAndDrawableDataPtr = vertexAndDrawableDataPtr;
#ifdef TRIANGLES
	outBarycentricCoords = vec3(1,0,0);
#endif
#ifdef LINES
	outBarycentricCoords = vec2(1,0);
#endif
	outVertexPosition3 = eyePosition0.xyz / eyePosition0.w;

	// normal
	uint normalAccessInfo = getNormalAccessInfo();
	if(normalAccessInfo != 0)
		outVertexNormal = normalize(mat3(modelViewMatrix) * readVec3(vertex0DataPtr, normalAccessInfo));
	else
		outVertexNormal = vec3(0,0,-1);

	// tangent
	uint tangentAccessInfo = getTangentAccessInfo();
	if(tangentAccessInfo != 0)
		outVertexTangent = normalize(mat3(modelViewMatrix) * readVec3(vertex0DataPtr, tangentAccessInfo));
	else
		outVertexTangent = vec3(1,0,0);

#ifdef ID_BUFFER
	outId = inId[0];
#endif

	// finish first vertex
	EmitVertex();


	// second vertex

	// multiplication by projection "matrix"
#if 1
	gl_Position.x = scene.p11*eyePosition1.x + p31*eyePosition1.z + p41*eyePosition1.w;
	gl_Position.y = scene.p22*eyePosition1.y + p32*eyePosition1.z + p42*eyePosition1.w;
	gl_Position.z = scene.p33*eyePosition1.z + scene.p43*eyePosition1.w;
	gl_Position.w = p34*eyePosition1.z + p44*eyePosition1.w;
#else
	gl_Position = scene.projectionMatrix * eyePosition1;
#endif

	// set output variables
	outVertexAndDrawableDataPtr = vertexAndDrawableDataPtr;
#ifdef TRIANGLES
	outBarycentricCoords = vec3(0,1,0);
#endif
#ifdef LINES
	outBarycentricCoords = vec2(0,1);
#endif
	outVertexPosition3 = eyePosition1.xyz / eyePosition1.w;

	// normal
	if(normalAccessInfo != 0)
		outVertexNormal = normalize(mat3(modelViewMatrix) * readVec3(vertex1DataPtr, normalAccessInfo));
	else
		outVertexNormal = vec3(0,0,-1);

	// tangent
	if(tangentAccessInfo != 0)
		outVertexTangent = normalize(mat3(modelViewMatrix) * readVec3(vertex1DataPtr, tangentAccessInfo));
	else
		outVertexTangent = vec3(1,0,0);

#ifdef ID_BUFFER
	outId = inId[1];
#endif

	// finish second vertex
	EmitVertex();


	// third vertex

#ifdef TRIANGLES

	// multiplication by projection "matrix"
#if 1
	gl_Position.x = scene.p11*eyePosition2.x + p31*eyePosition2.z + p41*eyePosition2.w;
	gl_Position.y = scene.p22*eyePosition2.y + p32*eyePosition2.z + p42*eyePosition2.w;
	gl_Position.z = scene.p33*eyePosition2.z + scene.p43*eyePosition2.w;
	gl_Position.w = p34*eyePosition2.z + p44*eyePosition2.w;
#else
	gl_Position = scene.projectionMatrix * eyePosition2;
#endif

	// set output variables
	outVertexAndDrawableDataPtr = vertexAndDrawableDataPtr;
	outBarycentricCoords = vec3(0,0,1);
	outVertexPosition3 = eyePosition2.xyz / eyePosition2.w;

	// normal
	if(normalAccessInfo != 0)
		outVertexNormal = normalize(mat3(modelViewMatrix) * readVec3(vertex2DataPtr, normalAccessInfo));
	else
		outVertexNormal = vec3(0,0,-1);

	// tangent
	if(tangentAccessInfo != 0)
		outVertexTangent = normalize(mat3(modelViewMatrix) * readVec3(vertex2DataPtr, tangentAccessInfo));
	else
		outVertexTangent = vec3(1,0,0);

#ifdef ID_BUFFER
	outId = inId[2];
#endif

	// finish first vertex
	EmitVertex();

#endif

}
