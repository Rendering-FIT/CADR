#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : require
#extension GL_GOOGLE_include_directive : require
#include "UberShaderReadFuncs.glsl"
#include "UberShaderInterface.glsl"


// output to fragment shader
layout(location = 0, component = 0) out flat uint64_t outVertexDataPtr;
layout(location = 0, component = 2) out flat uint64_t outDrawableDataPtr;
layout(location = 1) out smooth vec3 outVertexPosition3;  // in eye coordinates
layout(location = 2) out smooth vec3 outVertexNormal;  // in eye coordinates
layout(location = 3) out smooth vec3 outVertexTangent;  // in eye coordinates
#ifdef ID_BUFFER
layout(location = 4) out flat uvec2 outId;
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
	// DrawablePointers
	DrawablePointersRef dp = DrawablePointersRef(drawablePointersBufferPtr + (gl_DrawID * DrawablePointersSize));
	outDrawableDataPtr = dp.drawableDataPtr;

	// model matrix list
	MatrixListRef modelMatrixList = MatrixListRef(dp.matrixListPtr);

	// vertex data
	IndexDataRef indexData = IndexDataRef(dp.indexDataPtr);
	uint vertexDataSize = getVertexDataSize();
	uint index = indexData.indices[gl_VertexIndex];
	uint64_t vertexDataPtr = dp.vertexDataPtr + (index * vertexDataSize);
	outVertexDataPtr = vertexDataPtr;

	// vertex positions
	// (it is stored on offset 0 by convention)
	uint positionAccessInfo = getPositionAccessInfo();
	vec3 position = readVec3(positionAccessInfo, vertexDataPtr);

	// matrices and positions
	SceneDataRef scene = SceneDataRef(sceneDataPtr);
	mat4 modelViewMatrix = scene.viewMatrix * modelMatrixList.matrices[gl_InstanceIndex];
	vec4 eyePosition = modelViewMatrix * vec4(position, 1);

	// multiplication by projection "matrix"
#if 1
	gl_Position.x = scene.p11*eyePosition.x + p31*eyePosition.z + p41*eyePosition.w;
	gl_Position.y = scene.p22*eyePosition.y + p32*eyePosition.z + p42*eyePosition.w;
	gl_Position.z = scene.p33*eyePosition.z + scene.p43*eyePosition.w;
	gl_Position.w = p34*eyePosition.z + p44*eyePosition.w;
#else
	gl_Position = scene.projectionMatrix * eyePosition;
#endif

	// point size
	gl_PointSize = pointSize;

	// set vertex position in eye coordinates
	outVertexPosition3 = eyePosition.xyz / eyePosition.w;

	// normal
	uint normalAccessInfo = getNormalAccessInfo();
	if(normalAccessInfo != 0)
		outVertexNormal = normalize(mat3(modelViewMatrix) * readVec3(normalAccessInfo, vertexDataPtr));
	else
		outVertexNormal = vec3(0,0,-1);

	// tangent
	uint tangentAccessInfo = getTangentAccessInfo();
	if(tangentAccessInfo != 0)
		outVertexTangent = normalize(mat3(modelViewMatrix) * readVec3(tangentAccessInfo, vertexDataPtr));
	else
		outVertexTangent = vec3(1,0,0);

#ifdef ID_BUFFER
	outId.x = gl_DrawID;
	outId.y = gl_InstanceIndex;
#endif

}
