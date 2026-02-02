#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : require
#extension GL_EXT_nonuniform_qualifier : require  // this enables SPV_EXT_descriptor_indexing
#extension GL_GOOGLE_include_directive : require
#include "UberShaderReadFuncs.glsl"
#include "UberShaderInterface.glsl"


// input and output
#if defined(TRIANGLES) || defined(LINES)
layout(location = 0) in flat u64vec4 inVertexAndDrawableDataPtrList;  // VertexData on indices 0..2 for triangles and 0..1 for lines. DrawableData on index 3. The vector occupies locations 0 and 1.
# ifdef TRIANGLES
layout(location = 2) in smooth vec3 inBarycentricCoords;  // barycentric coordinates using perspective correction
# endif
# ifdef LINES
layout(location = 2) in smooth vec2 inBarycentricCoords;  // barycentric coordinates using perspective correction
# endif
layout(location = 3) in smooth vec3 inFragmentPosition3;  // in eye coordinates
layout(location = 4) in smooth vec3 inFragmentNormal;  // in eye coordinates
layout(location = 5) in smooth vec3 inFragmentTangent;  // in eye coordinates
# ifdef ID_BUFFER
layout(location = 6) in flat uvec2 inId;
# endif
#endif

#ifdef POINTS
layout(location = 0, component = 0) in flat uint64_t inVertexDataPtr;
layout(location = 0, component = 2) in flat uint64_t inDrawableDataPtr;
layout(location = 1) in smooth vec3 inFragmentPosition3;  // in eye coordinates
layout(location = 2) in smooth vec3 inFragmentNormal;  // in eye coordinates
layout(location = 3) in smooth vec3 inFragmentTangent;  // in eye coordinates
# ifdef ID_BUFFER
layout(location = 4) in flat uvec2 inId;
# endif
#endif

layout(location = 0) out vec4 outColor;
#ifdef ID_BUFFER
layout(location = 1) out uvec4 outId;
#endif


// textures
layout(set=0, binding=0) uniform sampler2D textureList[];
#if 0
layout(set=0, binding=0) uniform sampler2D baseColorTexture;  // RGB encoded with sRGB transfer function, A represents linear alpha, Hoops uses diffuseTexture (using MaterialKit) and baseColorMap (using PBRMaterialKit)
layout(set=0, binding=1) uniform sampler2D metallicRoughnessTexture;  // B - metalness, G - roughness, both encoded with linear transfer function, Hoops uses MetalnessMap (in PBRMaterialKit) and RoughnessMap (in PBRMaterialKit)
layout(set=0, binding=2) uniform sampler2D normalTexture;  // RGB represents XYZ components in tangent space encoded with linear transfer function, Hoops uses bump texture (in MaterialKit) and NormalMap (in PBRMaterialKit)
layout(set=0, binding=3) uniform sampler2D occlusionTexture;  // R contains occlusion values using linear transfer function, Hoops uses OcclusionMap (in PBRMaterialKit)
layout(set=0, binding=4) uniform sampler2D emissiveTexture;  // RGB represents color and intensity of the light being emitted encoded with sRGB transfer function, Hoops uses emission texture (using MaterialKit) and emissiveMap (using PBRMaterialKit)
// from extensions:
layout(set=0, binding=5) uniform sampler2D anisotropyTexture;  // RG represents anisotropy direction, B contains anisotropy strength
layout(set=0, binding=6) uniform sampler2D clearcoatTexture;
layout(set=0, binding=7) uniform sampler2D clearcoatRoughnessTexture;
layout(set=0, binding=8) uniform sampler2D clearcoatNormalTexture;
layout(set=0, binding=9) uniform sampler2D iridescenceTexture;  // R - iridiscence intensity
layout(set=0, binding=10) uniform sampler2D iridescenceThicknessTexture;  // G - thickness
layout(set=0, binding=11) uniform sampler2D sheenColorTexture;  // RGB encoded with sRGB transfer function
layout(set=0, binding=12) uniform sampler2D sheenRoughnessTexture;  // A contains roughness
layout(set=0, binding=13) uniform sampler2D specularTexture;  // A contains strength of the specular reflection, Hoops uses specular channel
layout(set=0, binding=14) uniform sampler2D specularColorTexture;  // RGB specifies F0 color of the specular reflection encoded as sRGB
layout(set=0, binding=15) uniform sampler2D transmissionTexture;  // R - transmission percentage, Hoops uses transmission texture in MaterialKit
layout(set=0, binding=16) uniform sampler2D thicknessTexture;  // G - thickness
// from Hoops:
layout(set=0, binding=17) uniform samplerCube environmentCubeMap;
layout(set=0, binding=18) uniform samplerCube environmentTexture;
layout(set=0, binding=19) uniform sampler2D mirrorTexture;
#endif


//               Viewer  Normal  Halfway vector
//     Reflected     V      N      H           Light source
//        light       V     N     H         LLL
//           RRR       V    N    H       LLL
//              RRR     V   N   H     LLL
//                 RRR   V  N  H   LLL
//                    RRR V N H LLL
//                       RRVNHLL
//      SSSSSSSSSSSSSSSSSSS F SSSSSSSSSSSSSSSSSSS
//
//  S - Surface of the primitive
//  F - Fragment being rendered
//  N - Normal - normalized normal at Fragment's surface position
//  V - Viewer - normalized vector from Fragment to Viewer
//  L - Light - normalized vector from Fragment to Light source
//  H - Halfway vector
//  R - Reflected light direction - normalized vector
//
//  All vectors are in eye coordinates.

void OpenGLDirectionalLight(
	in LightRef lightData,
	in vec3 normal,
	in vec3 viewerToFragmentDirection,
	in float shininess,
	inout vec3 ambient,
	inout vec3 diffuse,
	inout vec3 specular)
{
	// nDotL = normal . light direction
	vec3 l = lightData.positionOrDirection;  // directional light uses direction towards the incoming light here
	float nDotL = dot(normal, l);

	if(nDotL > 0.) {

		// Lambertian diffuse reflection
		diffuse += nDotL * lightData.opengl.diffuse;

		// nDotH = normal . halfway vector
		vec3 h = normalize(l - viewerToFragmentDirection);
		float nDotH = dot(normal, h);

		if(nDotH > 0.) {

			// specular term and its power factor
			float pf = pow(nDotH, shininess);
			specular = pf * lightData.opengl.specular;

		}
	}

	ambient += lightData.opengl.ambient;
}


void OpenGLPointLight(
	in LightRef lightData,
	in vec3 normal,
	in vec3 viewerToFragmentDirection,
	in float shininess,
	inout vec3 ambient,
	inout vec3 diffuse,
	inout vec3 specular)
{
	// nDotL = normal . light direction
	vec3 lPos = lightData.positionOrDirection;  // point light uses position of the light source in eye coordinates here
	lPos -= inFragmentPosition3;  // make lPos vector from fragment to light
	float lLen = length(lPos);
	vec3 lDir = lPos / lLen;  // direction from the fragment to the light source
	float nDotL = dot(normal, lDir);

	// attenuation
	float att = 1. / (lightData.opengl.constantAttenuation +
		lightData.opengl.linearAttenuation * lLen +
		lightData.opengl.quadraticAttenuation * lLen * lLen);

	if(nDotL > 0.) {

		// Lambertian diffuse reflection
		diffuse += nDotL * lightData.opengl.diffuse * att;

		// nDotH = normal . halfway vector
		vec3 h = normalize(lDir - viewerToFragmentDirection);
		float nDotH = dot(normal, h);

		if(nDotH > 0.) {

			// specular term and its power factor
			float pf = pow(nDotH, shininess);
			specular = pf * lightData.opengl.specular * att;

		}
	}

	ambient += lightData.opengl.ambient * att;
}


void OpenGLSpotlight(
	in LightRef lightData,
	in vec3 normal,
	in vec3 viewerToFragmentDirection,
	in float shininess,
	inout vec3 ambient,
	inout vec3 diffuse,
	inout vec3 specular)
{
	// light position and direction
	vec3 lPos = lightData.positionOrDirection;  // point light uses position of the light source in eye coordinates here
	lPos -= inFragmentPosition3;  // make lPos vector from fragment to light
	float lLen = length(lPos);
	vec3 lDir = lPos / lLen;  // direction from the fragment to the light source

	// skip everything outside of spotlight outer cone
	float spotEffect = dot(-lDir, lightData.spotlight.direction);
	if(spotEffect > lightData.spotlight.cosOuterConeAngle) {

		// compute spotEffect
		if(lightData.spotlight.cosInnerConeAngle == -1.) {

			// OpenGL spotlight
			spotEffect = pow(spotEffect, lightData.spotlight.spotExponent);

		} else {

			// DirectX style spotlight
			spotEffect =
				pow(
					clamp(
						(spotEffect - lightData.spotlight.cosOuterConeAngle) /
						(lightData.spotlight.cosInnerConeAngle - lightData.spotlight.cosOuterConeAngle),
						0., 1.),
					lightData.spotlight.spotExponent
				);

			// Hermite interpolation
			// (result will be between 0 and 1)
			spotEffect = smoothstep(0., 1., spotEffect);

		}

		// nDotL = normal . light direction
		float nDotL = dot(normal, lDir);

		// attenuation
		float att = 1. / (lightData.opengl.constantAttenuation +
			lightData.opengl.linearAttenuation * lLen +
			lightData.opengl.quadraticAttenuation * lLen * lLen);

		if(nDotL > 0.) {

			// Lambertian diffuse reflection
			diffuse += nDotL * lightData.opengl.diffuse * att * spotEffect;

			// nDotH = normal . halfway vector
			vec3 h = normalize(lDir - viewerToFragmentDirection);
			float nDotH = dot(normal, h);

			if(nDotH > 0.) {

				// specular term and its power factor
				float pf = pow(nDotH, shininess);
				specular = pf * lightData.opengl.specular * att * spotEffect;

			}
		}

		ambient += lightData.opengl.ambient * att * spotEffect;
	}
}


void main()
{
	// input data and structures
	SceneDataRef scene = SceneDataRef(sceneDataPtr);
	vec3 viewerToFragmentDirection = normalize(inFragmentPosition3);
#ifdef TRIANGLES
	u64vec3 vertexDataPtrList = inVertexAndDrawableDataPtrList.xyz;
	uint64_t drawableDataPtr = inVertexAndDrawableDataPtrList.w;
#endif
#ifdef LINES
	u64vec2 vertexDataPtrList = inVertexAndDrawableDataPtrList.xy;
	uint64_t drawableDataPtr = inVertexAndDrawableDataPtrList.w;
#endif
#ifdef POINTS
	uint64_t vertexDataPtr = inVertexDataPtr;
	uint64_t drawableDataPtr = inDrawableDataPtr;
#endif

	// normal
	vec3 normal;
	uint materialModel = getMaterialModel();
	if(materialModel >= 1) {
		if(getGenerateFlatNormals())
			normal = -normalize(cross(dFdx(inFragmentPosition3), dFdy(inFragmentPosition3)));
		else
			normal = normalize(inFragmentNormal);
#ifdef TRIANGLES
		if(getMaterialTwoSidedLighting() && !gl_FrontFacing)
			normal = -normal;
#endif
	}

	// init textureType and textureInfo
	uint textureOffset = getMaterialFirstTextureOffset();
	uint textureType;
	TextureInfoRef textureInfo;
	if(textureOffset == 0)
		textureType = 0;
	else {
		textureInfo = TextureInfoRef(drawableDataPtr + textureOffset);
		textureType = textureInfo.texCoordIndexTypeAndSettings & 0xff00;
	}

	// normal texture
	if(textureType == 0x0100) {

		// compute texture coordinates from relevant data,
		// and transform them if requested
#if defined(TRIANGLES) || defined(LINES)
		vec2 uv = computeTextureCoordinates(textureInfo, vertexDataPtrList, inBarycentricCoords);
#else
		vec2 uv = computeTextureCoordinates(textureInfo, vertexDataPtr);
#endif

		// sample texture
		vec3 tangentSpaceNormal = texture(textureList[textureInfo.textureIndex], uv).rgb;

		// transform in tangent space and normalize
		tangentSpaceNormal = tangentSpaceNormal * 2 - 1;  // transform from 0..1 to -1..1
		if(getTextureUseStrengthFlag(textureInfo))
			tangentSpaceNormal.xy *= textureInfo.strength;
		tangentSpaceNormal = normalize(tangentSpaceNormal);

		// transform normal
		vec3 t = normalize(inFragmentTangent);
		mat3 tbn = { t, cross(normal, t), normal };
		normal = tbn * tangentSpaceNormal;

		// update pointer to point to the next texture
		textureInfo = getNextTextureInfo(textureInfo);
		textureType = textureInfo.texCoordIndexTypeAndSettings & 0xff00;
	}

	// unlit material
	if(materialModel == 0) {

		// material data
		UnlitMaterialRef unlitMaterial = UnlitMaterialRef(drawableDataPtr);

		uint colorAccessInfo = getColorAccessInfo();
		if(colorAccessInfo != 0) {

			// read attribute, processa alpha
			if(getMaterialIgnoreColorAttributeAlpha()) {
#if defined(TRIANGLES) || defined(LINES)
				outColor.rgb = interpolateAttribute3(colorAccessInfo,
					vertexDataPtrList, inBarycentricCoords);
#else
				outColor.rgb = readVec3(colorAccessInfo, vertexDataPtr);
#endif
				if(getMaterialIgnoreMaterialAlpha())
					outColor.a = 1;
				else
					outColor.a = unlitMaterial.colorAndAlpha.a;
			} else {
#if defined(TRIANGLES) || defined(LINES)
				outColor = interpolateAttribute4(colorAccessInfo,
					vertexDataPtrList, inBarycentricCoords);
#else
				outColor = readVec4(colorAccessInfo, vertexDataPtr);
#endif
				if(!getMaterialIgnoreMaterialAlpha())
					outColor.a = unlitMaterial.colorAndAlpha.a;
			}

			// multiply color attribute with material
			if(getUnlitMaterialMultiplyColorAttributeWithMaterial())
				outColor.rgb *= unlitMaterial.colorAndAlpha.rgb;

		}
		else
		{
			// set color using material
			if(getMaterialIgnoreMaterialAlpha())
				outColor = vec4(unlitMaterial.colorAndAlpha.rgb, 1);
			else
				outColor = unlitMaterial.colorAndAlpha;
		}

		if(textureType == 0x0400) {

			// compute texture coordinates from relevant data,
			// and transform them if requested
#if defined(TRIANGLES) || defined(LINES)
			vec2 uv = computeTextureCoordinates(textureInfo, vertexDataPtrList, inBarycentricCoords);
#else
			vec2 uv = computeTextureCoordinates(textureInfo, vertexDataPtr);
#endif

			// sample texture
			vec4 baseTextureValue = texture(textureList[textureInfo.textureIndex], uv);

			// multiply by strength
			if(getTextureUseStrengthFlag(textureInfo))
				baseTextureValue *= textureInfo.strength;

			// apply texture using texEnv
			uint texEnv = getTextureEnvironment(textureInfo);
			if(getMaterialIgnoreBaseTextureAlpha()) {
				if(texEnv == 0)  // modulate
					outColor.rgb *= baseTextureValue.rgb;
				else if(texEnv == 1) // replace
					outColor.rgb = baseTextureValue.rgb;
				else if(texEnv == 2) // decal
					outColor.rgb = baseTextureValue.rgb;
				else if(texEnv == 3) // blend
					outColor.rgb = outColor.rgb*(1-baseTextureValue.rgb) + getTextureBlendColor(textureInfo)*baseTextureValue.rgb;
				else if(texEnv == 3) // add
					outColor.rgb = outColor.rgb + baseTextureValue.rgb;
			} else {
				if(texEnv == 0)  // modulate
					outColor *= baseTextureValue;
				else if(texEnv == 1) // replace
					outColor = vec4(baseTextureValue.rgb, baseTextureValue.a * outColor.a);
				else if(texEnv == 2) // decal
					outColor = vec4(outColor.rgb*(1-baseTextureValue.a) + baseTextureValue.rgb*baseTextureValue.a, outColor.a);
				else if(texEnv == 3) // blend
					outColor = vec4(outColor.rgb*(1-baseTextureValue.rgb) + getTextureBlendColor(textureInfo)*baseTextureValue.rgb, outColor.a*baseTextureValue.a);
				else if(texEnv == 3) // add
					outColor = vec4(outColor.rgb + baseTextureValue.rgb, outColor.a * baseTextureValue.a);
			}

			// update pointer to point to the next texture
			textureInfo = getNextTextureInfo(textureInfo);
			textureType = textureInfo.texCoordIndexTypeAndSettings & 0xff00;

		}

	}


	// Blinn-Phong material model,
	// implemented in the similar was as OpenGL does
	else if(materialModel == 1) {

		// occlusion texture
		float occlusionTextureValue = 1.;
		if(textureType == 0x0200) {

			// compute texture coordinates from relevant data,
			// and transform them if requested
#if defined(TRIANGLES) || defined(LINES)
			vec2 uv = computeTextureCoordinates(textureInfo, vertexDataPtrList, inBarycentricCoords);
#else
			vec2 uv = computeTextureCoordinates(textureInfo, vertexDataPtr);
#endif

			// sample texture
			uint componentIndex = getTextureFirstComponentIndex(textureInfo);  // glTF uses red component, so 0 should be provided for glTF
			float occlusionTextureValue = texture(textureList[textureInfo.textureIndex], uv)[componentIndex];

			// multiply by strength
			if(getTextureUseStrengthFlag(textureInfo))
				occlusionTextureValue *= textureInfo.strength;

			// update pointer to point to the next texture
			textureInfo = getNextTextureInfo(textureInfo);
			textureType = textureInfo.texCoordIndexTypeAndSettings & 0xff00;

		}

		// emissive texture
		vec3 emissiveTextureValue = vec3(1);
		if(textureType == 0x0300) {

			// compute texture coordinates from relevant data,
			// and transform them if requested
#if defined(TRIANGLES) || defined(LINES)
			vec2 uv = computeTextureCoordinates(textureInfo, vertexDataPtrList, inBarycentricCoords);
#else
			vec2 uv = computeTextureCoordinates(textureInfo, vertexDataPtr);
#endif

			// sample texture
			emissiveTextureValue = texture(textureList[textureInfo.textureIndex], uv).rgb;

			// multiply by strength
			if(getTextureUseStrengthFlag(textureInfo))
				emissiveTextureValue *= textureInfo.strength;

			// update pointer to point to the next texture
			textureInfo = getNextTextureInfo(textureInfo);
			textureType = textureInfo.texCoordIndexTypeAndSettings & 0xff00;

		}

		// material data
		PhongMaterialRef phongMaterial = PhongMaterialRef(drawableDataPtr);

		// ambientColor, diffuseColor and alpha
		vec3 ambientColor;
		vec3 diffuseColor;
		uint colorAccessInfo = getColorAccessInfo();
		if(colorAccessInfo != 0) {

			// get diffuseColor and initialize outColor.a
			if(getMaterialIgnoreColorAttributeAlpha()) {
#if defined(TRIANGLES) || defined(LINES)
				diffuseColor = interpolateAttribute3(colorAccessInfo,
					vertexDataPtrList, inBarycentricCoords);
#else
				diffuseColor = readVec3(colorAccessInfo, vertexDataPtr);
#endif
				if(getMaterialIgnoreMaterialAlpha())
					outColor.a = 1;
				else
					outColor.a = phongMaterial.diffuseAndAlpha.a;
			} else {
#if defined(TRIANGLES) || defined(LINES)
				vec4 color = interpolateAttribute4(colorAccessInfo,
					vertexDataPtrList, inBarycentricCoords);
#else
				vec4 color = readVec4(colorAccessInfo, vertexDataPtr);
#endif
				diffuseColor = color.rgb;
				outColor.a = color.a;
				if(!getMaterialIgnoreMaterialAlpha())
					outColor.a *= phongMaterial.diffuseAndAlpha.a;
			}

			// multiply color attribute with material
			if(getPhongMaterialApplyColorAttributeOnDiffuseOnly()) {
				ambientColor = phongMaterial.ambient;
				if(getPhongMaterialMultiplyColorAttributeWithMaterial())
					diffuseColor *= phongMaterial.diffuseAndAlpha.rgb;
			}
			else {
				ambientColor = diffuseColor;
				if(getPhongMaterialMultiplyColorAttributeWithMaterial()) {
					diffuseColor *= phongMaterial.diffuseAndAlpha.rgb;
					ambientColor *= phongMaterial.ambient;
				}
			}

		}
		else
		{
			// set color using material
			ambientColor = phongMaterial.ambient;
			diffuseColor = phongMaterial.diffuseAndAlpha.rgb;
			if(getMaterialIgnoreMaterialAlpha())
				outColor.a = 1;
			else
				outColor.a = phongMaterial.diffuseAndAlpha.a;
		}

		// light data
		uint64_t lightDataPtr = sceneDataPtr + getLightDataOffset();
		LightRef lightData = LightRef(lightDataPtr);

		// iterate over all light sources
		uint lightSettings = lightData.settings;
		if(lightSettings != 0) {

			// Phong color products
			vec3 ambientProduct  = vec3(0);
			vec3 diffuseProduct  = vec3(0);
			vec3 specularProduct = vec3(0);

			// iterate over all lights
			do{

				uint lightType = getLightType(lightSettings);
				if(lightType == 1)
					OpenGLDirectionalLight(lightData, normal,
						viewerToFragmentDirection, phongMaterial.shininess,
						ambientProduct, diffuseProduct, specularProduct);
				else if(lightType == 2)
					OpenGLPointLight(lightData, normal,
						viewerToFragmentDirection, phongMaterial.shininess,
						ambientProduct, diffuseProduct, specularProduct);
				else
					OpenGLSpotlight(lightData, normal,
						viewerToFragmentDirection, phongMaterial.shininess,
						ambientProduct, diffuseProduct, specularProduct);

				lightDataPtr += getLightDataSize();
				lightData = LightRef(lightDataPtr);
				lightSettings = lightData.settings;

			} while(lightSettings != 0);

			// Phong equation
			outColor.rgb = phongMaterial.emission * emissiveTextureValue +
			               (ambientProduct + scene.ambientLight) * ambientColor * occlusionTextureValue +
			               diffuseProduct * diffuseColor +
			               specularProduct * phongMaterial.specular;

		}
		else {

			// Phong equation without light sources
			outColor.rgb = phongMaterial.emission * emissiveTextureValue +
			               scene.ambientLight * ambientColor * occlusionTextureValue;

		}

		if(textureType == 0x0400) {

			// compute texture coordinates from relevant data,
			// and transform them if requested
#if defined(TRIANGLES) || defined(LINES)
			vec2 uv = computeTextureCoordinates(textureInfo, vertexDataPtrList, inBarycentricCoords);
#else
			vec2 uv = computeTextureCoordinates(textureInfo, vertexDataPtr);
#endif

			// sample texture
			vec4 baseTextureValue = texture(textureList[textureInfo.textureIndex], uv);

			// multiply by strength
			if(getTextureUseStrengthFlag(textureInfo))
				baseTextureValue *= textureInfo.strength;

			// apply texture using texEnv
			uint texEnv = getTextureEnvironment(textureInfo);
			if(getMaterialIgnoreBaseTextureAlpha()) {
				if(texEnv == 0)  // modulate
					outColor.rgb *= baseTextureValue.rgb;
				else if(texEnv == 1) // replace
					outColor.rgb = baseTextureValue.rgb;
				else if(texEnv == 2) // decal
					outColor.rgb = baseTextureValue.rgb;
				else if(texEnv == 3) // blend
					outColor.rgb = outColor.rgb*(1-baseTextureValue.rgb) + getTextureBlendColor(textureInfo)*baseTextureValue.rgb;
				else if(texEnv == 3) // add
					outColor.rgb = outColor.rgb + baseTextureValue.rgb;
			} else {
				if(texEnv == 0)  // modulate
					outColor *= baseTextureValue;
				else if(texEnv == 1) // replace
					outColor = vec4(baseTextureValue.rgb, baseTextureValue.a * outColor.a);
				else if(texEnv == 2) // decal
					outColor = vec4(outColor.rgb*(1-baseTextureValue.a) + baseTextureValue.rgb*baseTextureValue.a, outColor.a);
				else if(texEnv == 3) // blend
					outColor = vec4(outColor.rgb*(1-baseTextureValue.rgb) + getTextureBlendColor(textureInfo)*baseTextureValue.rgb, outColor.a*baseTextureValue.a);
				else if(texEnv == 3) // add
					outColor = vec4(outColor.rgb + baseTextureValue.rgb, outColor.a * baseTextureValue.a);
			}

			// update pointer to point to the next texture
			textureInfo = getNextTextureInfo(textureInfo);
			textureType = textureInfo.texCoordIndexTypeAndSettings & 0xff00;

		}

	}

	// Metallic-Roughness material model from PBR family,
	// implemented in the similar way as glTF does
	else if(materialModel == 2) {

		// light data
		uint64_t lightDataPtr = sceneDataPtr + getLightDataOffset();
		LightRef lightData = LightRef(lightDataPtr);

		// iterate over all light sources
		uint lightSettings = lightData.settings;
		if(lightSettings != 0) {

			// material data
			MetallicRoughnessMaterialRef metallicRoughnessMaterial = MetallicRoughnessMaterialRef(drawableDataPtr);

			do{

				lightDataPtr += getLightDataSize();
				lightData = LightRef(lightDataPtr);
				lightSettings = lightData.settings;

			} while(lightSettings != 0);

		}
		else {
		}

	}


	// write id-buffer
#ifdef ID_BUFFER
	outId[0] = stateSetID;
	outId[1] = inId[0];  // gl_DrawID - index of indirect drawing structure
	outId[2] = inId[1];  // gl_InstanceIndex
	outId[3] = gl_PrimitiveID;
#endif
}
