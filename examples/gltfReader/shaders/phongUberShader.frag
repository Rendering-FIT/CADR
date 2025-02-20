#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : require


// per-drawable shader data
layout(buffer_reference, std430, buffer_reference_align=64) restrict readonly buffer ShaderDataRef {
	layout(offset= 0) vec4 ambientAndSettings;
	layout(offset=16) vec4 diffuseAndAlpha;
	layout(offset=32) vec3 specular;
	layout(offset=44) float shininess;
	layout(offset=48) vec3 emission;
	layout(offset=60) float pointSize;
	layout(offset=64) mat4 modelMatrix[];
};


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

layout(push_constant) uniform pushConstants {
	layout(offset=8)  uint64_t sceneDataPtr;  // one buffer for the whole scene
	layout(offset=16) uint stateSetID;  // ID of the current StateSet
};


#ifdef TEXTURING
layout(set=0, binding=0) uniform sampler2D baseTexture;
#endif


// input and output
layout(location = 0) flat in uint64_t inDataPtr;
layout(location = 1) smooth in vec3 inEyePosition3;
layout(location = 2) smooth in vec3 inEyeNormal;
#ifdef PER_VERTEX_COLOR
layout(location = 3) smooth in vec4 inColor;
# ifdef TEXTURING
layout(location = 4) smooth in vec2 inTexCoord;
#  ifdef ID_BUFFER
layout(location = 5) flat in uvec2 inId;
#  endif
# else
#  ifdef ID_BUFFER
layout(location = 4) flat in uvec2 inId;
#  endif
# endif
#else
# ifdef TEXTURING
layout(location = 3) smooth in vec2 inTexCoord;
#  ifdef ID_BUFFER
layout(location = 4) flat in uvec2 inId;
#  endif
# else
#  ifdef ID_BUFFER
layout(location = 3) flat in uvec2 inId;
#  endif
# endif
#endif

layout(location = 0) out vec4 outColor;
#ifdef ID_BUFFER
layout(location = 1) out uvec4 outId;
#endif


void directionalLight(in SceneDataRef scene,
                      in int i,
                      in vec3 eyePositionDir,
                      in vec3 normal,
                      in float shininess,
                      inout vec3 ambient,
                      inout vec3 diffuse,
                      inout vec3 specular)
{
	float nDotL; // normal . light direction
	float nDotH; // normal . light half vector direction
	float pf;    // power factor

	vec3 l = scene.lights[i].eyePositionDir;
	nDotL = dot(normal, l);

	if(nDotL > 0.)
	{
		diffuse  += scene.lights[i].diffuse * nDotL;

		vec3 h = normalize(l - eyePositionDir);
		nDotH = dot(normal, h);
		if(nDotH > 0.) {
			pf = pow(nDotH, shininess);
			specular += scene.lights[i].specular * pf;
		}
	}

	ambient  += scene.lights[i].ambient;
}


void main(void)
{
	ShaderDataRef data = ShaderDataRef(inDataPtr);
	SceneDataRef scene = SceneDataRef(sceneDataPtr);

	// settings
	//
	// the meaning of bits:
	// 0..3 - mode (see bellow)
	// 4..5 - color settings
	//          when no bit is set, ambient and diffuse are taken from inColor
	//          0x10 - ambientMaterial; ambient is taken from material instead of inColor
	//          0x20 - diffuseMaterial; diffuse is taken from material instead of inColor
	//          if PER_VERTEX_COLOR is not defined, ambient and diffuse is always taken from material
	// 6..8 - alpha settings
	//          when no bit is set, alpha is taken from the material
	//          0x40 - noMaterialAlpha
	//          0x80 - inColorAlpha
	//          0x100 - textureAlpha
	//             this produces eight combinations:
	//                0x000 - materialAlpha, 0x040 - alpha set to 1.0,
	//                0x080 - materialAlpha*inColorAlpha, 0x0c0 - inColorAlpha
	//                0x100 - materialAlpha*textureAlpha, 0x140 - textureAlpha
	//                0x180 - materialAlpha*inColorAlpha*textureAlpha, 0x1c0 - inColorAlpha*textureAlpha
	//          if TEXTURING is not defined, texture alpha is not applied
	//
	uint settings = floatBitsToInt(data.ambientAndSettings.w);

	// mode
	//
	// the meaning of bits:
	// BaseColor = 0x02;
	// BaseTexture = 0x03;
	// Phong = 0x04;
	// TexturedPhong = 0x05;
	//
	uint mode = settings & 0x0f;

#ifdef TEXTURING

	// base texture
	vec4 textureColor;
	if((mode & 0x01) != 0)
		textureColor = texture(baseTexture, vec2(inTexCoord));

#endif

	// compute alpha
	outColor.a = ((settings & 0x40) != 0) ? 1.0 : data.diffuseAndAlpha.a;
#ifdef PER_VERTEX_COLOR
	if((settings & 0x80) != 0)
		outColor.a *= inColor.a;
#endif

	if(mode <= 0x03) {

#ifdef TEXTURING

		// surface color given by baseTexture
		if(mode == 0x03 ) {

			// return BaseColor texture
			// with properly computed alpha
			if((settings & 0x100) != 0)
				outColor *= textureColor;
			else
				outColor.rgb *= textureColor.rgb;
			return;

		}

#endif

		// return baseColor
#ifdef PER_VERTEX_COLOR
		if((settings & 0x30) == 0)
			outColor.rgb = data.diffuseAndAlpha.rgb;
		else
			outColor.rgb = inColor.rgb;
#else
		outColor.rgb = data.diffuseAndAlpha.rgb;
#endif
		return;

	}
	else
	{

		// vectors for Phong
		vec3 eyePositionDir = normalize(inEyePosition3);
		vec3 normal = normalize(inEyeNormal);
		if(!gl_FrontFacing)
			normal = -normal;

		// Phong color products
		vec3 ambientProduct  = vec3(0);
		vec3 diffuseProduct  = vec3(0);
		vec3 specularProduct = vec3(0);
		for(int i=0; i<scene.numLights; i++)
			directionalLight(scene, i, eyePositionDir, normal, data.shininess,
			                 ambientProduct, diffuseProduct, specularProduct);

	#ifdef PER_VERTEX_COLOR
		vec3 ambient = ((settings & 0x10) != 0) ? data.ambientAndSettings.rgb : inColor.rgb;
		vec3 diffuse = ((settings & 0x20) != 0) ? data.diffuseAndAlpha.rgb : inColor.rgb;
	#else
		vec3 ambient = data.ambientAndSettings.rgb;
		vec3 diffuse = data.diffuseAndAlpha.rgb;
	#endif

		outColor.rgb = data.emission + (scene.ambientLight * ambient) +
		               (ambientProduct * ambient) +
		               (diffuseProduct * diffuse) +
		               (specularProduct * data.specular);

#ifdef TEXTURING

		// combine with baseTexture
		if((mode & 0x01) != 0)
		{
			if((settings & 0x100) != 0)
				outColor *= textureColor;
			else
				outColor.rgb *= textureColor.rgb;
		}

#endif

	}


#ifdef ID_BUFFER
	outId[0] = stateSetID;
	outId[1] = inId[0];  // gl_DrawID - index of indirect drawing structure
	outId[2] = inId[1];  // gl_InstanceIndex
	outId[3] = gl_PrimitiveID;
#endif
}
