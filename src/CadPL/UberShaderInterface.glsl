
//
// scene data
//

layout(buffer_reference, std430, buffer_reference_align=64) restrict readonly buffer
SceneDataRef {
	mat4 viewMatrix;        // current camera view matrix
	float p11,p22,p33,p43;  // projectionMatrix - members that depend on zNear and zFar clipping planes
	vec3 ambientLight;
	layout(offset=128) uint lightData[];  // array of OpenGLLight and GltfLight structures is stored here
};
uint getLightDataOffset()  { return 128; }

// push constants
layout(push_constant) uniform pushConstants {
	layout(offset=0) uint64_t drawablePointersBufferPtr;  // one buffer for the whole scene
	layout(offset=8) uint64_t sceneDataPtr;  // one buffer for the whole scene
	layout(offset=16) uint attribAccessInfoList[8];  // per-stateSet attribAccessInfo for 16 attribs
	layout(offset=48) uint materialSetup;
#ifdef ID_BUFFER
	layout(offset=52) uint stateSetID;  // ID of the current StateSet
#endif
};
uint getPositionAccessInfo()  { return attribAccessInfoList[0] & 0xffff; }
uint getNormalAccessInfo()  { return attribAccessInfoList[0] >> 16; }
uint getTangentAccessInfo()  { return attribAccessInfoList[1] & 0xffff; }
uint getColorAccessInfo()  { return attribAccessInfoList[1] >> 16; }
uint getTexCoordAccessInfo(uint attribIndex) { uint texCoordAccessInfo = attribAccessInfoList[attribIndex>>1]; if((attribIndex & 0x1) == 0) texCoordAccessInfo &= 0x0000ffff; else texCoordAccessInfo >>= 16; return texCoordAccessInfo; }

// materialSetup
// bits 0..1: 0 - reserved, 1 - unlit, 2 - phong, 3 - metallicRoughness
// bits 2..7: texture offset (0, 4, 8, 12, .....252)
// bit 8: use color attribute for ambient and diffuse; material ambient and diffuse values are ignored
// bit 9: use color attribute for diffuse; material diffuse value is ignored
// bit 10: ignore color attribute alpha if color attribute is used (if bit 8 or 9 is set)
// bit 11: ignore material alpha
// bit 12: ignore base texture alpha if base texture is used
uint getMaterialModel()  { return materialSetup & 0x03; }
uint getMaterialFirstTextureOffset()  { return materialSetup & 0xfc; }
bool getMaterialUseColorAttribute()  { return (materialSetup & 0x0300) != 0; }
bool getMaterialUseColorAttributeForAmbientAndDiffuse()  { return (materialSetup & 0x0100) != 0; }
bool getMaterialUseColorAttributeForDiffuseOnly()  { return (materialSetup & 0x0200) != 0; }
bool getMaterialIgnoreColorAttributeAlpha()  { return (materialSetup & 0x0400) != 0; }
bool getMaterialIgnoreMaterialAlpha()  { return (materialSetup & 0x0800) != 0; }
bool getMaterialIgnoreBaseTextureAlpha()  { return (materialSetup & 0x1000) != 0; }



//
// material structures
//

layout(buffer_reference, std430, buffer_reference_align=16) restrict readonly buffer
UnlitMaterialRef {
	layout(offset=0) vec4 colorAndAlpha;
	layout(offset=16)  float pointSize;
	// [... texture data starts at offset 20 ...]
};

layout(buffer_reference, std430, buffer_reference_align=16) restrict readonly buffer
PhongMaterialRef {
	layout(offset=0)  vec3 ambient;  //< ambient color might be ignored and replaced by diffuse color when specified by settings
	layout(offset=16) vec4 diffuseAndAlpha;  //< Hoops uses four floats for diffuse and alpha in MaterialKit
	layout(offset=32) vec3 specular;  //< Hoops uses specular color in MaterialKit
	layout(offset=44) float shininess;  //< Hoops uses gloss (1x float) in MaterialKit
	layout(offset=48) vec3 emission;  //< Hoops uses 4x float in MaterialKit
	layout(offset=60) float pointSize;
	layout(offset=64) vec3 reflection;  //< Hoops uses mirror in MaterialKit
	// [... texture data starts at offset 76 ...]
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
	layout(offset=60) float pointSize;
	layout(offset=64) float alphaCutoff;

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

	layout(offset=64) mat4 modelMatrix[];
};

layout(buffer_reference, std430, buffer_reference_align=4) restrict readonly buffer
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
	//   bit 16 - multiply texture value by strength member; if neither bit 16 nor bit 17 is set,
	//            the TextureInfoRef structure occupies only 8 bytes and following TextureInfo
	//            structure is placed on the address incremented by 8 bytes only
	//   bit 17 - transform texture coordinates by the transformation specified by t1..t6;
	//            if bits 16 and 17 are not set, the structure occupies only 8 bytes;
	//            if bit 16 is set and bit 17 not, the structure occupies 12 bytes; otherwise
	//            it occupies 36 bytes; these sizes should be used to compute address
	//            of the next TextureInfo structure
	//   bit 18 - blend color included in the structure
	//   bits 19..21 - for Phong and its base texture, texture environment:
	//                 0 - modulate, 1 - replace, 2 - decal, 3 - blend, 4 - add
	//   bits 22..23 - first component index;
	uint texCoordIndexTypeAndSettings;
	uint textureIndex;
	float strength;
	float rs1,rs2,rs3,rs4,t1,t2;  // rotation and scale in 2x2 matrix, translation in vec2
	float blendR,blendG,blendB;  // texture blend color used in blend texture environment
};

bool getTextureUseStrengthFlag(TextureInfoRef textureInfo)  { return (textureInfo.texCoordIndexTypeAndSettings & 0x10000) != 0; }
bool getTextureTranformFlag(TextureInfoRef textureInfo)  { return (textureInfo.texCoordIndexTypeAndSettings & 0x20000) != 0; }
uint getTextureEnvironment(TextureInfoRef textureInfo)  { return (textureInfo.texCoordIndexTypeAndSettings >> 19) & 0x7; }
vec3 getTextureBlendColor(TextureInfoRef textureInfo)  { return vec3(textureInfo.blendR, textureInfo.blendG, textureInfo.blendB); }
uint getTextureFirstComponentIndex(TextureInfoRef textureInfo)  { return (textureInfo.texCoordIndexTypeAndSettings >> 22) & 0x3; }
uint getTexCoordAccessInfo(TextureInfoRef textureInfo) { return getTexCoordAccessInfo(textureInfo.texCoordIndexTypeAndSettings & 0xff); }

TextureInfoRef getNextTextureInfo(TextureInfoRef textureInfo)
{
	if((textureInfo.texCoordIndexTypeAndSettings & 0x70000) == 0)
		return TextureInfoRef(uint64_t(textureInfo) + 8);
	else if((textureInfo.texCoordIndexTypeAndSettings & 0x60000) == 0)
		return TextureInfoRef(uint64_t(textureInfo) + 12);
	else if((textureInfo.texCoordIndexTypeAndSettings & 0x40000) == 0)
		return TextureInfoRef(uint64_t(textureInfo) + 36);
	else
		return TextureInfoRef(uint64_t(textureInfo) + 48);
}

vec2 transformTexCoord(vec2 tc, TextureInfoRef textureInfo)
{
	mat2x2 rotationAndScale = {
		{ textureInfo.rs1, textureInfo.rs2, },
		{ textureInfo.rs3, textureInfo.rs4, },
	};
	vec2 translation = { textureInfo.t1, textureInfo.t2, };
	return rotationAndScale * tc + translation;
}

vec2 computeTextureCoordinates(TextureInfoRef textureInfo, uint64_t vertex0DataPtr,
	uint64_t vertex1DataPtr, uint64_t vertex2DataPtr, vec3 barycentricCoords)
{
	// get texture coordinates
	uint texCoordAccessInfo = getTexCoordAccessInfo(textureInfo);
	vec2 uv0 = readVec2(vertex0DataPtr, texCoordAccessInfo);
	vec2 uv1 = readVec2(vertex1DataPtr, texCoordAccessInfo);
	vec2 uv2 = readVec2(vertex2DataPtr, texCoordAccessInfo);
	vec2 uv = uv0 * barycentricCoords.x + uv1 * barycentricCoords.y +
	          uv2 * barycentricCoords.z;

	// transform texture coordinates
	if(getTextureTranformFlag(textureInfo))
		uv = transformTexCoord(uv, textureInfo);

	return uv;
}



//
// light source
//

struct OpenGLLightData {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float constantAttenuation;
	float linearAttenuation;
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
	uint settings;  // switches between point light, directional light and spotlight
	layout(offset=16) OpenGLLightData opengl;
	layout(offset=80) GltfLightData gltf;
	layout(offset=112) SpotlightData spotlight;
};
uint getLightDataSize()  { return 192; }



//
// drawable data
//

layout(buffer_reference, std430, buffer_reference_align=64) restrict readonly buffer
MatrixListRef {
	mat4 matrices[];
};

// indices
layout(buffer_reference, std430, buffer_reference_align=4) restrict readonly buffer
IndexDataRef {
	uint indices[];
};

layout(buffer_reference, std430, buffer_reference_align=64) restrict readonly buffer
DrawableDataRef {
	// bit 6..8: modelMatrix offset (0, 64, 128, 192,..., 448)
	uint settings1;
	// bit 2..8: vertex data size (0, 4, 8,..., 508)
	uint settings2;
	uint64_t materialPtr;
	// [... model matrices ...]
};

MatrixListRef getDrawableMatrixList(uint64_t drawableDataPtr)  { return MatrixListRef(drawableDataPtr + (DrawableDataRef(drawableDataPtr).settings1 & 0x01c0)); }
uint getDrawableVertexDataSize(uint64_t drawableDataPtr)  { return DrawableDataRef(drawableDataPtr).settings2 & 0x01fc; }

// drawable data pointers
layout(buffer_reference, std430, buffer_reference_align=8) restrict readonly buffer DrawablePointersRef {
	uint64_t vertexDataPtr;
	uint64_t indexDataPtr;
	uint64_t drawableDataPtr;
};
const uint DrawablePointersSize = 24;
