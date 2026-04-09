// SPDX-FileCopyrightText: 2025-2026 PCJohn (Jan Pečiva, peciva@fit.vut.cz)
//
// SPDX-License-Identifier: MIT


//
// scene data
//

layout(buffer_reference, std430, buffer_reference_align=64) restrict readonly buffer
SceneDataRef {
	mat4 viewMatrix;        // current camera view matrix
	mat4 projectionMatrix;  // current camera projection matrix
	float p11,p22,p33,p43;  // alternative specification of projectionMatrix - only members that depend on zNear
	                        // and zFar clipping planes; remaining members are passed in as specialization constants
	vec3 ambientLight;      // scene ambient light
	layout(offset=192) uint lightData[];  // array of OpenGLLight and GltfLight structures is stored here
};
uint getLightDataOffset()  { return 192; }



//
// push constants
//

layout(push_constant) uniform
pushConstants {
	layout(offset=0) uint64_t sceneDataPtr;  // pointer to SceneDataRef; usually updated per scene render pass or once per scene rendering
	layout(offset=8) uint64_t drawablePointersBufferPtr;  // pointer to DrawablePointersRef array; there is one DrawablePointersRef array for each StateSet, so the pointer is updated before each StateSet rendering
	layout(offset=16) uint attribAccessInfoList[8];  // per-stateSet attribAccessInfo for 16 attribs
	layout(offset=48) uint attribSetup;  // doc is provided bellow with getVertexDataSize()
	layout(offset=52) uint materialSetup;  // doc is provided with UnlitMaterialRef, PhongMaterialRef and MetallicRoughnessMaterialRef
#ifdef POINTS
	layout(offset=56) float pointSize;  // ignored unless rendering points; it specifies the size of the points that will be rendered
#endif
#ifdef ID_BUFFER
	layout(offset=60) uint stateSetID;  // ID of the current StateSet
#endif
	layout(offset=64) uint textureSetup[10];  // each uint holds texCoordIndex, type and settings of a texture
	layout(offset=104) uint64_t lightSetup;  // 1 byte for each light; it holds lightType
};

// pushConstants.attribAccessInfoList
uint getPositionAccessInfo()  { return attribAccessInfoList[0] & 0xffff; }
uint getNormalAccessInfo()  { return attribAccessInfoList[0] >> 16; }
uint getTangentAccessInfo()  { return attribAccessInfoList[1] & 0xffff; }
uint getColorAccessInfo()  { return attribAccessInfoList[1] >> 16; }
uint getTexCoordAccessInfo(uint attribIndex) { uint texCoordAccessInfo = attribAccessInfoList[attribIndex>>1]; if((attribIndex & 0x1) == 0) texCoordAccessInfo &= 0x0000ffff; else texCoordAccessInfo >>= 16; return texCoordAccessInfo; }

// pushConstants.attribSetup
// bit 2..8: vertex data size (0, 4, 8,..., 508)
uint getVertexDataSize()  { return attribSetup & 0x01fc; }
bool getGenerateFlatNormals()  { return (attribSetup & 0x0001) != 0; }

// pushConstants.materialSetup
// bits 0..1: material model; 0 - unlit, 1 - phong, 2 - metallicRoughness
// bits 2..7: material texturing params offset (0, 4, 8, 12, .....252)
// bit 8: two sided lighting
// bit 9: disable lighting;
//        for unlit model, no effect;
//        for phong model, use sum of diffuse and emission components;
//        for metallic-roughness model, sum of base color and emissive value is used;
//        textures can still be used, but some might not have any visual effect;
//        to optimize for performance, textures not affecting visual output shall be turned off
// bit 10: use alpha test; alphaCutoff is specified in material; when computed alpha is greater
//         than alphaCutoff, fragment is rendered as fully opaque; otherwise, it is rendered as
//         fully transparent
// bit 11: ignore color attribute alpha if color attribute is used
// bit 12: ignore material alpha
// bit 13: ignore base texture alpha if base texture is used
// unlit:
//    bit 14: ignored
//    bit 15: glTF or OpenGL handling of color attribute;
//            if set, color attribute is multiplied with material color
//            during base color computation; this follows glTF design;
//            if not set, color attribute is used directly for base color,
//            ignoring material color; this follows OpenGL fixed pipeline design;
//            if color attribute is not present, material color is used and this bit has no effect
//    bit 16: ignored
// phong:
//    bit 14: apply color attribute on ambient and diffuse, or on diffuse only;
//            if set, color attribute is applied just on diffuse component;
//            otherwise, it is applied on ambient and diffuse
//    bit 15: glTF or OpenGL handling of color attribute with relation to diffuse and ambient material;
//            if set, color attribute is multiplied with material diffuse (and ambient, see bellow) color
//            when computing diffuse light contribution; this follows glTF design;
//            if not set, only color attribute is used during diffuse (and ambient, see bellow) light computation,
//            ignoring diffuse (and ambient) material color; this follows OpenGL fixed pipeline design;
//            if bit 10 is set, the same applies for ambient component
//            (final ambient = color attribute * material ambient);
//    bit 16 - for Phong: glTF or OpenGL style material emission color;
//                        It switches material emission color application either
//                        after base color texture or before base color texture.
//                        If the bit 12 is set to zero, OpenGL style is used.
//                        Material emission color is applied in the same way as in fixed OpenGL pipeline,
//                        e.g. material emission color contributes to the final color computed
//                        by the lighting equation while base color texture is applied on this final color.
//                        If emissive texture is active, it is added at the end to the final fragment shader output.
//                        If the bit 12 is set to one, glTF style is used.
//                        Material emission color is not involved in any color calculations
//                        except that it is multiplied by emissive texture, if used, and added
//                        to the final fragment shader output.
uint getMaterialModel()  { return materialSetup & 0x03; }
uint getMaterialTexturingParamsOffset()  { return materialSetup & 0xfc; }
bool getMaterialTwoSidedLighting()  { return (materialSetup & 0x0100) != 0; }
bool getMaterialDisableLighting()  { return (materialSetup & 0x0200) != 0; }
bool getMaterialAlphaTest()  { return (materialSetup & 0x0400) != 0; }
bool getMaterialIgnoreColorAttributeAlpha()  { return (materialSetup & 0x0800) != 0; }
bool getMaterialIgnoreMaterialAlpha()  { return (materialSetup & 0x1000) != 0; }
bool getMaterialIgnoreBaseTextureAlpha()  { return (materialSetup & 0x2000) != 0; }
bool getUnlitMaterialMultiplyColorAttributeWithMaterial()  { return (materialSetup & 0x8000) != 0; }
bool getPhongMaterialApplyColorAttributeOnDiffuseOnly()  { return (materialSetup & 0x4000) != 0; }
bool getPhongMaterialMultiplyColorAttributeWithMaterial()  { return (materialSetup & 0x8000) != 0; }
bool getPhongMaterialOpenGLStyleEmission()  { return (materialSetup & 0x10000) == 0; }
bool getPhongMaterialSeparateEmission()  { return (materialSetup & 0x10000) != 0; }

// pushConstants.textureSetup[10];
// texCoordIndex - bits 0..7
// type - bits 8..15
//   0 - marks the end of TextureInfo array; TextureInfo array is ended by
//       texCoordIndexTypeAndSettings set to zero
//   1 - normal texture
//   2 - occusion texture
//   3 - emissive texture
//   4 - base texture
// settings - bits 16..31
//   bit 16 - transform texture coordinates by the transformation specified by t1..t6
//   bit 17 - multiply texture value by strength member
//   bits 18..20 - for Phong's base texture only, texture environment:
//                 0 - modulate, 1 - replace, 2 - decal, 3 - blend, 4 - add;
//                 note: when blend texture environment is used (e.g. bits 18..20 are set to 3),
//                       blendColor is included in textureInfo and occupies extra 12 bytes
uint getTextureCoordinateIndex(uint textureIndex)  { return textureSetup[textureIndex] & 0x00ff; }
uint getTextureTypeShL8(uint textureIndex)  { return textureSetup[textureIndex] & 0xff00; }
bool getTextureUseCoordinateTranform(uint textureIndex)  { return (textureSetup[textureIndex] & 0x10000) != 0; }
bool getTextureUseStrength(uint textureIndex)  { return (textureSetup[textureIndex] & 0x20000) != 0; }
uint getTextureEnvironment(uint textureIndex)  { return (textureSetup[textureIndex] >> 18) & 0x7; }



//
// material structures
//

layout(buffer_reference, std430, buffer_reference_align=16) restrict readonly buffer
UnlitMaterialRef {
	layout(offset=0) vec4 colorAndAlpha;
	layout(offset=16) float alphaCutoff;  //< optional alphaCutoff member; cutoff treshold when using mask alpha mode; alpha above this threshold is rendered as fully opaque and alpha bellow this threshold is rendered as fully transparent
	// [... texturing params usually start at offset 16,
	// just when alphaCutoff is included, it starts at offset 20 ...]
};

layout(buffer_reference, std430, buffer_reference_align=16) restrict readonly buffer
PhongMaterialRef {
	layout(offset=0)  vec3 ambient;  //< ambient color
	layout(offset=16) vec4 diffuseAndAlpha;  //< diffuse color and alpha
	layout(offset=32) vec3 specular;  //< specular color
	layout(offset=44) float shininess;  //< shininess (or gloss)
	layout(offset=48) vec3 emission;  //< emitted light
	layout(offset=60) float alphaCutoff;  //< cutoff treshold when using mask alpha mode; alpha above this threshold is rendered as fully opaque and alpha bellow this threshold is rendered as fully transparent
#if 0
	layout(offset=64) vec3 reflection;  //< amount of mirroring reflection
#endif
	// [... texturing params start at offset 64 or at offset 76 if reflection is used ...]
};

layout(buffer_reference, std430, buffer_reference_align=64) restrict readonly buffer
MetallicRoughnessMaterialRef {
	layout(offset=0) vec4 baseColorFactor;
	layout(offset=16) float alphaCutoff;
	layout(offset=20) float metallicFactor;
	layout(offset=24) float roughnessFactor;
	layout(offset=32) vec3 emissiveFactor;

	// 25 additional floats (100 bytes)
	float anisotropyStrength;
	float anisotropyRotation;
	float clearcoatFactor;
	float clearcoatRoughnessFactor;
	float dispersion;
	float emissiveStrength;
	float ior;
	float iridescenceFactor;
	float iridescenceIor;
	float iridescenceThicknessMinimum;
	float iridescenceThicknessMaximum;
	vec3 sheenColorFactor;
	float sheenRoughnessFactor;
	float specularFactor;
	vec3 specularColorFactor;
	float transmissionFactor;
	float thicknessFactor;
	float attenuationDistance;
	vec3 attenuationColor;
};


// Material texturing params
//
// Data of each texture are shown bellow. Many of them are optional.
// The presence of optional members can be determined by pushConstant.textureSetup.
// The data items are sorted in the order as they are needed in the shader.
//
// optional: float rs1,rs2,rs3,rs4,t1,t2;  // rotation and scale in 2x2 matrix (rs1..rs4), translation in vec2 (t1,t2)
// uint textureID;  // index of the texture in the descriptor array
// optional: float strength;  // strength of the effect (of occlusion or emissive texture,...),
//                            // or scale (of normals in normal texture)
// optional: vec3 blendColor;  // blend color used in blend texture environment
//
// The offset of each member depends on the presence or absence of all optional members that precede this one;
// those that are not present do not occupy any space, e.g. their space is occupied by the following members

uint getTextureIdAndUpdatePtr(inout uint64_t ptr)  { uint r=AlignedUIntRef(ptr).value; ptr+=4; return r; }
float getTextureStrengthAndUpdatePtr(inout uint64_t ptr)  { float r=AlignedFloatRef(ptr).value; ptr+=4; return r; }
vec3 getTextureBlendColorAndUpdatePtr(inout uint64_t ptr)  { vec3 r=UnalignedVec3Ref(ptr).value; ptr+=12; return r; }

layout(buffer_reference, std430, buffer_reference_align=4) restrict readonly buffer
TextureTransformDataRef {
	vec4 rotationAndScale;
	vec2 translation;
};

vec2 transformTexCoordAndUpdatePtr(vec2 tc, inout uint64_t ptr)
{
	TextureTransformDataRef tt = TextureTransformDataRef(ptr);
	mat2x2 rotationAndScale = {
		{ tt.rotationAndScale[0], tt.rotationAndScale[1], },
		{ tt.rotationAndScale[2], tt.rotationAndScale[3], },
	};
	vec2 translation = { tt.translation[0], tt.translation[1], };
	ptr += 24;
	return rotationAndScale * tc + translation;
}

vec2 computeTextureCoordinatesAndUpdatePtr(uint textureIndex,
	u64vec3 vertexDataPtrList, vec3 barycentricCoords, inout uint64_t ptr)
{
	// get texture coordinates
	uint texCoordAccessInfo = getTexCoordAccessInfo(getTextureCoordinateIndex(textureIndex));
	vec2 uv0 = readVec2(texCoordAccessInfo, vertexDataPtrList[0]);
	vec2 uv1 = readVec2(texCoordAccessInfo, vertexDataPtrList[1]);
	vec2 uv2 = readVec2(texCoordAccessInfo, vertexDataPtrList[2]);

	// interpolate
	vec2 uv = uv0 * barycentricCoords.x + uv1 * barycentricCoords.y +
	          uv2 * barycentricCoords.z;

	// transform texture coordinates
	if(getTextureUseCoordinateTranform(textureIndex))
		uv = transformTexCoordAndUpdatePtr(uv, ptr);

	return uv;
}

vec2 computeTextureCoordinatesAndUpdatePtr(uint textureIndex,
	u64vec2 vertexDataPtrList, vec2 barycentricCoords, inout uint64_t ptr)
{
	// get texture coordinates
	uint texCoordAccessInfo = getTexCoordAccessInfo(getTextureCoordinateIndex(textureIndex));
	vec2 uv0 = readVec2(texCoordAccessInfo, vertexDataPtrList[0]);
	vec2 uv1 = readVec2(texCoordAccessInfo, vertexDataPtrList[1]);

	// interpolate
	vec2 uv = uv0 * barycentricCoords.x + uv1 * barycentricCoords.y;

	// transform texture coordinates
	if(getTextureUseCoordinateTranform(textureIndex))
		uv = transformTexCoordAndUpdatePtr(uv, ptr);

	return uv;
}

vec2 computeTextureCoordinatesAndUpdatePtr(uint textureIndex,
	uint64_t vertexDataPtr, inout uint64_t ptr)
{
	// get texture coordinates
	uint texCoordAccessInfo = getTexCoordAccessInfo(getTextureCoordinateIndex(textureIndex));
	vec2 uv = readVec2(texCoordAccessInfo, vertexDataPtr);

	// transform texture coordinates
	if(getTextureUseCoordinateTranform(textureIndex))
		uv = transformTexCoordAndUpdatePtr(uv, ptr);

	return uv;
}

vec3 interpolateAttribute3(uint attribAccessInfo,
	u64vec3 vertexDataPtrList, vec3 barycentricCoords)
{
	return readVec3(attribAccessInfo, vertexDataPtrList[0]) * barycentricCoords.x +
	       readVec3(attribAccessInfo, vertexDataPtrList[1]) * barycentricCoords.y +
	       readVec3(attribAccessInfo, vertexDataPtrList[2]) * barycentricCoords.z;
}

vec4 interpolateAttribute4(uint attribAccessInfo,
	u64vec3 vertexDataPtrList, vec3 barycentricCoords)
{
	return readVec4(attribAccessInfo, vertexDataPtrList[0]) * barycentricCoords.x +
	       readVec4(attribAccessInfo, vertexDataPtrList[1]) * barycentricCoords.y +
	       readVec4(attribAccessInfo, vertexDataPtrList[2]) * barycentricCoords.z;
}

vec3 interpolateAttribute3(uint attribAccessInfo,
	u64vec2 vertexDataPtrList, vec2 barycentricCoords)
{
	return readVec3(attribAccessInfo, vertexDataPtrList[0]) * barycentricCoords.x +
	       readVec3(attribAccessInfo, vertexDataPtrList[1]) * barycentricCoords.y;
}

vec4 interpolateAttribute4(uint attribAccessInfo,
	u64vec2 vertexDataPtrList, vec2 barycentricCoords)
{
	return readVec4(attribAccessInfo, vertexDataPtrList[0]) * barycentricCoords.x +
	       readVec4(attribAccessInfo, vertexDataPtrList[1]) * barycentricCoords.y;
}



//
// light source
//

struct OpenGLLightData {
	vec3 ambient;
	float constantAttenuation;
	vec3 diffuse;
	float linearAttenuation;
	vec3 specular;
	float quadraticAttenuation;
};

struct GltfLightData {
	vec3 color;
	float intensity;  // for point light and spotlight in candelas (lm/sr), and for directional light in luxes (lm/m2)
	float range;
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
};

struct SpotlightData {
	vec3 direction;  // spotlight direction in eye coordinates, it must be normalized
#if 0  // linear interpolation or OpenGL style spotlight
	float cosOuterConeAngle;  // cosinus of outer spotlight cone; outside the cone, there is zero light intensity
	float cosInnerConeAngle;  // cosinus of inner spotlight cone; if -1. is provided, OpenGL-style spotlight is used, ignoring inner cone and using spotExponent instead; if value is > -1., DirectX style spotlight is used, e.g. everything inside the inner cone receives full light intensity and light intensity between inner and outer cone is linearly interpolated starting from zero intensity on outer code to full intensity in inner cone
	float spotExponent;  // if cosInnerConeAngle is -1, OpenGL style spotlight is used, using spotExponent
	uint padding[2];
#else  // glTF recommended interpolation
	float angleScale;  // angleScale = 1.0f / max(0.001f, cos(innerConeAngle) - cos(outerConeAngle));
	float angleOffset;  // angleOffset = -cos(outerConeAngle) * lightAngleScale;
	uint padding[3];
#endif
};

layout(buffer_reference, std430, buffer_reference_align=64) restrict readonly buffer
LightRef {
	layout(offset=0)  vec3 positionOrDirection;  // for point light and spotlight: position in eye coordinates,
	                                             // for directional light: direction in eye coordinates, direction must be normalized
	layout(offset=12) uint settings;  // docs provided bellow
	layout(offset=16) OpenGLLightData opengl;
	layout(offset=64) GltfLightData gltf;
	layout(offset=96) SpotlightData spotlight;
};
uint getLightDataSize()  { return 128; }

// LightRef.settings
// all bits zero - no light, used as the last light in the light list
// bits 0..1: 1 - directional light, 2 - point light, 3 - spotlight
uint getLightType(uint lightSettings)  { return lightSettings & 0x3; }



//
// drawable data
//

layout(buffer_reference, std430, buffer_reference_align=64) restrict readonly buffer
MatrixListRef {
	uint numMatrices;
	uint capacity;
	uint64_t padding[7];
	mat4 matrices[];
};

// indices
layout(buffer_reference, std430, buffer_reference_align=4) restrict readonly buffer
IndexDataRef {
	uint indices[];
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
