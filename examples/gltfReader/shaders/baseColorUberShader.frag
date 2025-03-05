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


// scene data
layout(buffer_reference, std430, buffer_reference_align=64) restrict readonly buffer SceneDataRef {
	mat4 viewMatrix;        // current camera view matrix
	float p11,p22,p33,p43;  // projectionMatrix - members that depend on zNear and zFar clipping planes
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
#ifdef PER_VERTEX_COLOR
layout(location = 2) smooth in vec4 inColor;
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
#else
# ifdef TEXTURING
layout(location = 2) smooth in vec2 inTexCoord;
#  ifdef ID_BUFFER
layout(location = 3) flat in uvec2 inId;
#  endif
# else
#  ifdef ID_BUFFER
layout(location = 2) flat in uvec2 inId;
#  endif
# endif
#endif

layout(location = 0) out vec4 outColor;
#ifdef ID_BUFFER
layout(location = 1) out uvec4 outId;
#endif


void main(void)
{
	ShaderDataRef data = ShaderDataRef(inDataPtr);

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
	//          when no bit is set, alpha is computed by multiplication of all possible elements,
	//             e.g. materialAlpha * inColorAlpha * textureAlpha, replacing all inactive elements by 1.0,
	//             if, for instance, no texture is active, or no color attribute is specified
	//          0x40 - noMaterialAlpha
	//          0x80 - noColorAlpha
	//          0x100 - noTextureAlpha
	//             this produces eight combinations:
	//                0x000 - materialAlpha*inColorAlpha*textureAlpha
	//                0x040 - inColorAlpha*textureAlpha
	//                0x080 - materialAlpha*textureAlpha
	//                0x0c0 - textureAlpha
	//                0x100 - materialAlpha*inColorAlpha
	//                0x140 - inColorAlpha
	//                0x180 - materialAlpha
	//                0x1c0 - alpha set to 1.0
	//          if TEXTURING is not defined, textureAlpha is replaced by 1.0
	//
	uint settings = floatBitsToInt(data.ambientAndSettings.w);

	// mode
	//
	// the meaning of bits:
	// Texture = 0x01
	// Color = 0x02
	// ColoredTexture = 0x03
	// Phong = 0x04 - not supported by this shader
	// TexturedPhong = 0x05 - not supported by this shader
	//
	uint mode = settings & 0x0f;

#ifdef TEXTURING

	// base texture
	vec4 textureColor;
	if((mode & 0x01) != 0)
		textureColor = texture(baseTexture, vec2(inTexCoord));

#endif

	// compute alpha
	outColor.a = ((settings & 0x40) == 0) ? data.diffuseAndAlpha.a : 1.0;  // noMaterialAlpha
#ifdef PER_VERTEX_COLOR
	if((settings & 0x80) == 0)  // noColorAlpha
		outColor.a *= inColor.a;
#endif

#ifdef TEXTURING

	// unlit texture
	if(mode == 0x01) {

		// unlit texture
		// with properly computed alpha
		if((settings & 0x100) == 0)  // noTextureAlpha
			outColor.a *= textureColor.a;
		outColor.rgb = textureColor.rgb;

	}
	else
	{

#endif

		// unlit color
#ifdef PER_VERTEX_COLOR
		if((settings & 0x30) == 0)
			outColor.rgb = inColor.rgb;
		else
			outColor.rgb = data.diffuseAndAlpha.rgb;
#else
		outColor.rgb = data.diffuseAndAlpha.rgb;
#endif

#ifdef TEXTURING

		// coloredTexture
		if(mode == 0x03) {
			if((settings & 0x100) == 0)  // noTextureAlpha
				outColor *= textureColor;
			else
				outColor.rgb *= textureColor.rgb;
		}
	}

#endif


#ifdef ID_BUFFER
	outId[0] = stateSetID;
	outId[1] = inId[0];  // gl_DrawID - index of indirect drawing structure
	outId[2] = inId[1];  // gl_InstanceIndex
	outId[3] = gl_PrimitiveID;
#endif
}
