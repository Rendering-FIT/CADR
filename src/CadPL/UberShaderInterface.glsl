
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
// bits 2..7: texture offset (0, 4, 8, 12, .....252)
// bit 8: two sided lighting
// bit 9: disable lighting; for phong model, use sum of diffuse and emission components;
//        for metallicRoughness, sum of base color and emissive value is used;
//        no effect for unlit model;
//        textures can still be used, but only those that have a meaning without lighting;
// unlit:
//    bit  9: ignored
//    bit 10: ignored
//    bit 11: glTF or OpenGL handling of color attribute;
//            if set, color attribute is multiplied with material color
//            during base color computation; this follows glTF design;
//            if not set, color attribute is used directly for base color,
//            ignoring material color; this follows OpenGL fixed pipeline design;
//            if color attribute is not present, material color is used and this bit has no effect
// phong:
//    bit 10: apply color attribute on ambient and diffuse, or on diffuse only;
//            if set, color attribute is applied just on diffuse component;
//            otherwise, it is applied on ambient and diffuse
//    bit 11: glTF or OpenGL handling of color attribute with relation to diffuse and ambient material;
//            if set, color attribute is multiplied with material diffuse (and ambient, see bellow) color
//            when computing diffuse light contribution; this follows glTF design;
//            if not set, only color attribute is used during diffuse (and ambient, see bellow) light computation,
//            ignoring diffuse (and ambient) material color; this follows OpenGL fixed pipeline design;
//            if bit 10 is set, the same applies for ambient component
//            (final ambient = color attribute * material ambient);
//    bit 12 - for Phong: glTF or OpenGL style material emission color;
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
// bit 13: ignore color attribute alpha if color attribute is used
// bit 14: ignore material alpha
// bit 15: ignore base texture alpha if base texture is used
uint getMaterialModel()  { return materialSetup & 0x03; }
uint getMaterialFirstTextureOffset()  { return materialSetup & 0xfc; }
bool getMaterialTwoSidedLighting()  { return (materialSetup & 0x0100) != 0; }
bool getMaterialDisableLighting()  { return (materialSetup & 0x0200) != 0; }
bool getUnlitMaterialMultiplyColorAttributeWithMaterial()  { return (materialSetup & 0x0800) != 0; }
bool getPhongMaterialApplyColorAttributeOnDiffuseOnly()  { return (materialSetup & 0x0400) != 0; }
bool getPhongMaterialMultiplyColorAttributeWithMaterial()  { return (materialSetup & 0x0800) != 0; }
bool getPhongMaterialOpenGLStyleEmission()  { return (materialSetup & 0x1000) == 0; }
bool getPhongMaterialSeparateEmission()  { return (materialSetup & 0x1000) != 0; }
bool getMaterialIgnoreColorAttributeAlpha()  { return (materialSetup & 0x2000) != 0; }
bool getMaterialIgnoreMaterialAlpha()  { return (materialSetup & 0x4000) != 0; }
bool getMaterialIgnoreBaseTextureAlpha()  { return (materialSetup & 0x8000) != 0; }



//
// material structures
//

layout(buffer_reference, std430, buffer_reference_align=16) restrict readonly buffer
UnlitMaterialRef {
	layout(offset=0) vec4 colorAndAlpha;
	// [... texture data starts at offset 16 ...]
};

layout(buffer_reference, std430, buffer_reference_align=16) restrict readonly buffer
PhongMaterialRef {
	layout(offset=0)  vec3 ambient;  //< ambient color
	layout(offset=16) vec4 diffuseAndAlpha;  //< diffuse color and alpha
	layout(offset=32) vec3 specular;  //< specular color
	layout(offset=44) float shininess;  //< shininess (or gloss)
	layout(offset=48) vec3 emission;  //< emitted light
	layout(offset=64) vec3 reflection;  //< amount of mirroring reflection
	// [... texture data starts at offset 76 or at offset 64 if reflection is not used ...]
};

layout(buffer_reference, std430, buffer_reference_align=64) restrict readonly buffer
MetallicRoughnessMaterialRef {
	// settings
	// bits 0..1: unlit, phong, metallicRoughness
	// bit 6..8: modelMatrix offset (0, 64, 128, 192,..., 448)
	// bit 9: use baseTexture
	layout(offset=0)  uvec4 settings;  // includes baseTextureCoordIndex, metallicRoughnessTextureCoordIndex,
	                                   // normalTextureCoordIndex, occlusionTextureCoordIndex,
	                                   // emissiveTextureCoordIndex, alphaMode, doubleSided, unlit,
	                                   // from extensions: anisotropyTextureCoordIndex
	layout(offset=16) vec4 baseColorFactor;  //< Hoops uses alpha and baseColor (in PBRMaterialKit)
	layout(offset=32) float metallicFactor;  //< Hoops uses metalnessFactor (in PBRMaterialKit)
	layout(offset=36) float roughnessFactor;  //< Hoops uses RoughnessFactor (in PBRMaterialKit)
	layout(offset=40) float normalTextureScale;  //< Hoops uses NormalFactor (in PBRMaterialKit)
	layout(offset=44) float occlusionTextureStrength;  //< Hoops uses OcclusionFactor (in PBRMaterialKit) with the same meaning
	layout(offset=48) vec3 emissiveFactor;
	layout(offset=60) float alphaCutoff;

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

layout(buffer_reference, std430, buffer_reference_align=8) restrict readonly buffer
TextureInfoRef {
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
	//   bits 18..20 - for Phong and its base texture, texture environment:
	//                 0 - modulate, 1 - replace, 2 - decal, 3 - blend, 4 - add;
	//                 note: when blend texture environment is used (e.g. bits 18..20 are set to 3),
	//                       blendColor is included in textureInfo and occupies extra 12 bytes
	//   bits 21..22 - first component index
	//   bits 26..31 - size of the structure (0..63 bytes), it must be multiple of 8;
	//                 structure size might be even 8 bytes if strength and the following
	//                 members are not used; size of the structure is used for computing
	//                 of the start address of the next TextureInfo structure
	uint texCoordIndexTypeAndSettings;
	uint textureIndex;
#if 0  // following members are optional;
       // their offsets depends on which members are actually present;
       // they are present in the specified order but those do not present does not occupy any space,
       // e.g. their space is occupied by the following members
	float rs1,rs2,rs3,rs4,t1,t2;  // optional: rotation and scale in 2x2 matrix (rs1..rs4), translation in vec2 (t1,t2); offset of rs1 is aligned to 8 bytes
	float strength;  // optional: strength of the effect (of occlusion or emissive texture,..),
	                 // or scale (of normals in normal texture)
	vec3 blendColor;  // optional: texture blend color used in blend texture environment
#endif
};

bool getTextureUseCoordinateTranform(TextureInfoRef textureInfo)  { return (textureInfo.texCoordIndexTypeAndSettings & 0x10000) != 0; }
bool getTextureUseStrength(TextureInfoRef textureInfo)  { return (textureInfo.texCoordIndexTypeAndSettings & 0x20000) != 0; }
uint getTextureEnvironment(TextureInfoRef textureInfo)  { return (textureInfo.texCoordIndexTypeAndSettings >> 18) & 0x7; }
uint getTextureFirstComponentIndex(TextureInfoRef textureInfo)  { return (textureInfo.texCoordIndexTypeAndSettings >> 21) & 0x3; }
uint getTexCoordAccessInfo(TextureInfoRef textureInfo) { return getTexCoordAccessInfo(textureInfo.texCoordIndexTypeAndSettings & 0xff); }

TextureInfoRef getNextTextureInfo(TextureInfoRef textureInfo)
{
	uint size = textureInfo.texCoordIndexTypeAndSettings >> 26;
	return TextureInfoRef(uint64_t(textureInfo) + size);
}

layout(buffer_reference, std430, buffer_reference_align=4) restrict readonly buffer
TextureStrengthRef {
	float value;
};

float getTextureStrength(uint64_t dataPointer)
{
	return TextureStrengthRef(dataPointer).value;
}

layout(buffer_reference, std430, buffer_reference_align=4) restrict readonly buffer
TextureBlendColorRef {
	vec3 value;
};

vec3 getTextureBlendColor(uint64_t dataPointer)
{
	return TextureBlendColorRef(dataPointer).value;
}

layout(buffer_reference, std430, buffer_reference_align=8) restrict readonly buffer
TextureTransformDataRef {
	vec4 rotationAndScale;
	vec2 translation;
};

vec2 transformTexCoord(vec2 tc, uint64_t dataPointer)
{
	TextureTransformDataRef tt = TextureTransformDataRef(dataPointer);
	mat2x2 rotationAndScale = {
		{ tt.rotationAndScale[0], tt.rotationAndScale[1], },
		{ tt.rotationAndScale[2], tt.rotationAndScale[3], },
	};
	vec2 translation = { tt.translation[0], tt.translation[1], };
	return rotationAndScale * tc + translation;
}

vec2 computeTextureCoordinates(TextureInfoRef textureInfo,
	u64vec3 vertexDataPtrList, vec3 barycentricCoords, out uint64_t nextDataPointer)
{
	// get texture coordinates
	uint texCoordAccessInfo = getTexCoordAccessInfo(textureInfo);
	vec2 uv0 = readVec2(texCoordAccessInfo, vertexDataPtrList[0]);
	vec2 uv1 = readVec2(texCoordAccessInfo, vertexDataPtrList[1]);
	vec2 uv2 = readVec2(texCoordAccessInfo, vertexDataPtrList[2]);

	// interpolate
	vec2 uv = uv0 * barycentricCoords.x + uv1 * barycentricCoords.y +
	          uv2 * barycentricCoords.z;

	// transform texture coordinates
	if(getTextureUseCoordinateTranform(textureInfo)) {
		uv = transformTexCoord(uv, uint64_t(textureInfo) + 8);
		nextDataPointer = uint64_t(textureInfo) + 32;
	} else
		nextDataPointer = uint64_t(textureInfo) + 8;

	return uv;
}

vec2 computeTextureCoordinates(TextureInfoRef textureInfo,
	u64vec2 vertexDataPtrList, vec2 barycentricCoords, out uint64_t nextDataPointer)
{
	// get texture coordinates
	uint texCoordAccessInfo = getTexCoordAccessInfo(textureInfo);
	vec2 uv0 = readVec2(texCoordAccessInfo, vertexDataPtrList[0]);
	vec2 uv1 = readVec2(texCoordAccessInfo, vertexDataPtrList[1]);

	// interpolate
	vec2 uv = uv0 * barycentricCoords.x + uv1 * barycentricCoords.y;

	// transform texture coordinates
	if(getTextureUseCoordinateTranform(textureInfo)) {
		uv = transformTexCoord(uv, uint64_t(textureInfo) + 8);
		nextDataPointer = uint64_t(textureInfo) + 32;
	} else
		nextDataPointer = uint64_t(textureInfo) + 8;

	return uv;
}

vec2 computeTextureCoordinates(TextureInfoRef textureInfo,
	uint64_t vertexDataPtr, out uint64_t nextDataPointer)
{
	// get texture coordinates
	uint texCoordAccessInfo = getTexCoordAccessInfo(textureInfo);
	vec2 uv = readVec2(texCoordAccessInfo, vertexDataPtr);

	// transform texture coordinates
	if(getTextureUseCoordinateTranform(textureInfo)) {
		uv = transformTexCoord(uv, uint64_t(textureInfo) + 8);
		nextDataPointer = uint64_t(textureInfo) + 32;
	} else
		nextDataPointer = uint64_t(textureInfo) + 8;

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
	float intensity;  // in candelas (lm/sr) for point light and spotlight and in luxes (lm/m2) for directional light
	float range;
};

struct SpotlightData {
	vec3 direction;  // spotlight direction in eye coordinates, it must be normalized
	float cosOuterConeAngle;  // cosinus of outer spotlight cone; outside the cone, there is zero light intensity
	float cosInnerConeAngle;  // cosinus of inner spotlight cone; if -1. is provided, OpenGL-style spotlight is used, ignoring inner cone and using spotExponent instead; if value is > -1., DirectX style spotlight is used, e.g. everything inside the inner cone receives full light intensity and light intensity between inner and outer cone is linearly interpolated starting from zero intensity on outer code to full intensity in inner cone
	float spotExponent;  // if cosInnerConeAngle is -1, OpenGL style spotlight is used, using spotExponent
};

layout(buffer_reference, std430, buffer_reference_align=64) restrict readonly buffer
LightRef {
	layout(offset=0)  vec3 positionOrDirection;  // for point light and spotlight: position in eye coordinates,
	                                             // for directional light: direction in eye coordinates, direction must be normalized
	uint settings;  // docs provided bellow
	layout(offset=16) OpenGLLightData opengl;
	layout(offset=80) GltfLightData gltf;
	layout(offset=112) SpotlightData spotlight;
};
uint getLightDataSize()  { return 192; }

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
