#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : require


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
//layout(set=0, binding=0) uniform sampler2D texUnit0;
//layout(set=0, binding=1) uniform sampler2D texUnit1;
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

	// texture 0
/*	vec4 textureColor;
	if( texture0Mode != 0 )
		textureColor = texture2D(texUnit0, gl_TexCoord[0]);


	// surface color
	if( texture0Mode == 0x1e01 ) // if GL_REPLACE

		// apply GL_REPLACE texture
		gl_FragColor = vec4(textureColor.rgb, textureColor.a*gl_Color.a);

	else*/
	{
	#ifdef PER_VERTEX_COLOR
		outColor.rgb = inColor.rgb;
		uint materialType = floatBitsToInt(data.ambientAndMaterialType.w);
		outColor.a = ((materialType & 0x40) != 0) ? inColor.a : data.diffuseAndAlpha.a;
	#else
		outColor.rgba = data.diffuseAndAlpha.rgba;
	#endif


		// apply texture 0
		/*if( texture0Mode != 0 )
		{
			if( texture0Mode == 0x2100 ) // GL_MODULATE
				gl_FragColor = gl_FragColor * textureColor;
			else if( texture0Mode == 0x2101 ) // GL_DECAL
				gl_FragColor = vec4(gl_FragColor.rgb*(1-textureColor.a) + textureColor.rgb*textureColor.a, gl_FragColor.a);
			else if( texture0Mode == 0x0BE2 ) // GL_BLEND
				gl_FragColor = vec4(gl_FragColor.rgb*(1-textureColor.rgb) + texture0BlendColor.rgb*textureColor.rgb, gl_FragColor.a*textureColor.a);
		}*/

	}


	// apply texture 1
	/*if( texture1Mode != 0 )
	{
		textureColor = texture2D(texUnit1, vec2(gl_TexCoord[0]));
		if( texture1Mode == 0x2100 ) // GL_MODULATE
			gl_FragColor = gl_FragColor * textureColor;
		else if( texture1Mode == 0x2101 ) // GL_DECAL
			gl_FragColor = vec4(gl_FragColor.rgb*(1-textureColor.a) + textureColor.rgb*textureColor.a, gl_FragColor.a);
		else if( texture1Mode == 0x0BE2 ) // GL_BLEND
			gl_FragColor = vec4(gl_FragColor.rgb*(1-textureColor.rgb) + texture1BlendColor.rgb*textureColor.rgb, gl_FragColor.a*textureColor.a);
	}*/

#ifdef ID_BUFFER
	outId[0] = stateSetID;
	outId[1] = inId[0];  // gl_DrawID - index of indirect drawing structure
	outId[2] = inId[1];  // gl_InstanceIndex
	outId[3] = gl_PrimitiveID;
#endif
}
