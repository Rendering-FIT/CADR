#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : require
#extension GL_GOOGLE_include_directive : require
#include "UberShaderReadFuncs.glsl"
#include "UberShaderInterface.glsl"

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;


// input from vertex shader
layout(location = 0) in flat int inDrawIndex[3];
layout(location = 1) in flat int inInstanceIndex[3];
layout(location = 2) in flat int inVertexIndex[3];
#ifdef ID_BUFFER
layout(location = 3) in flat uvec2 inId[3];
#endif

// output to fragment shader
layout(location = 0) out flat u64vec4 outVertexAndDrawableDataPtr;  // VertexData on indices 0..2 and ShaderData on index 3
layout(location = 1) out smooth vec3 outBarycentricCoords;  // barycentric coordinates using perspective correction
layout(location = 2) out smooth vec3 outVertexPosition3;  // in eye coordinates
layout(location = 3) out smooth vec3 outVertexNormal;  // in eye coordinates
layout(location = 4) out smooth vec3 outVertexTangent;  // in eye coordinates
#ifdef ID_BUFFER
layout(location = 5) out flat uvec2 outId;
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
	MatrixListRef modelMatrixList = getDrawableMatrixList(dp.drawableDataPtr);

	// vertex data
	IndexDataRef indexData = IndexDataRef(dp.indexDataPtr);
	uint index0 = indexData.indices[inVertexIndex[0]];
	uint index1 = indexData.indices[inVertexIndex[1]];
	uint index2 = indexData.indices[inVertexIndex[2]];
	uint vertexDataSize = getDrawableVertexDataSize(dp.drawableDataPtr);
	uint64_t vertex0DataPtr = dp.vertexDataPtr + (index0 * vertexDataSize);
	uint64_t vertex1DataPtr = dp.vertexDataPtr + (index1 * vertexDataSize);
	uint64_t vertex2DataPtr = dp.vertexDataPtr + (index2 * vertexDataSize);
	vertexAndDrawableDataPtr.x = vertex0DataPtr;
	vertexAndDrawableDataPtr.y = vertex1DataPtr;
	vertexAndDrawableDataPtr.z = vertex2DataPtr;

	// vertex positions
	// (it is stored on offset 0 by convention)
	uint positionAccessInfo = getPositionAccessInfo();
	vec3 position0 = readVec3(vertex0DataPtr, positionAccessInfo);
	vec3 position1 = readVec3(vertex1DataPtr, positionAccessInfo);
	vec3 position2 = readVec3(vertex2DataPtr, positionAccessInfo);

	// matrices and positions
	SceneDataRef scene = SceneDataRef(sceneDataPtr);
	mat4 modelViewMatrix = scene.viewMatrix * modelMatrixList.matrices[instanceIndex];
	vec4 eyePosition0 = modelViewMatrix * vec4(position0, 1);
	vec4 eyePosition1 = modelViewMatrix * vec4(position1, 1);
	vec4 eyePosition2 = modelViewMatrix * vec4(position2, 1);


	// first vertex

	// multiplication by projection "matrix"
	gl_Position.x = scene.p11*eyePosition0.x + p31*eyePosition0.z + p41*eyePosition0.w;
	gl_Position.y = scene.p22*eyePosition0.y + p32*eyePosition0.z + p42*eyePosition0.w;
	gl_Position.z = scene.p33*eyePosition0.z + scene.p43*eyePosition0.w;
	gl_Position.w = p34*eyePosition0.z + p44*eyePosition0.w;

	// set output variables
	outVertexAndDrawableDataPtr = vertexAndDrawableDataPtr;
	outBarycentricCoords = vec3(1,0,0);
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
	gl_Position.x = scene.p11*eyePosition1.x + p31*eyePosition1.z + p41*eyePosition1.w;
	gl_Position.y = scene.p22*eyePosition1.y + p32*eyePosition1.z + p42*eyePosition1.w;
	gl_Position.z = scene.p33*eyePosition1.z + scene.p43*eyePosition1.w;
	gl_Position.w = p34*eyePosition1.z + p44*eyePosition1.w;

	// set output variables
	outVertexAndDrawableDataPtr = vertexAndDrawableDataPtr;
	outBarycentricCoords = vec3(0,1,0);
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

	// multiplication by projection "matrix"
	gl_Position.x = scene.p11*eyePosition2.x + p31*eyePosition2.z + p41*eyePosition2.w;
	gl_Position.y = scene.p22*eyePosition2.y + p32*eyePosition2.z + p42*eyePosition2.w;
	gl_Position.z = scene.p33*eyePosition2.z + scene.p43*eyePosition2.w;
	gl_Position.w = p34*eyePosition2.z + p44*eyePosition2.w;

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

}
