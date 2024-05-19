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
//layout(set=0, binding=0) uniform sampler2D texUnit0;
//layout(set=0, binding=1) uniform sampler2D texUnit1;
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

	// texture 0
/*	vec4 textureColor;
	if( texture0Mode != 0 )
		textureColor = texture2D(texUnit0, vec2(gl_TexCoord[0]));


	// surface color
	if( texture0Mode == 0x1e01 ) // if GL_REPLACE

		// apply GL_REPLACE texture
		gl_FragColor = vec4(textureColor.rgb, textureColor.a*gl_Color.a);

	else*/
	{
		// Phong

		vec3 eyePositionDir = normalize(inEyePosition3);
		vec3 normal = normalize(inEyeNormal);
		if(!gl_FrontFacing)
			normal = -normal;

		vec3 ambientProduct  = vec3(0);
		vec3 diffuseProduct  = vec3(0);
		vec3 specularProduct = vec3(0);

		for(int i=0; i<scene.numLights; i++)
			directionalLight(scene, i, eyePositionDir, normal, data.shininess,
			                 ambientProduct, diffuseProduct, specularProduct);

	#ifdef PER_VERTEX_COLOR
		uint materialType = floatBitsToInt(data.ambientAndMaterialType.w);
		vec3 ambient = ((materialType & 0x20) == 0) ? inColor.rgb : data.ambientAndMaterialType.rgb;
		vec3 diffuse = inColor.rgb;
		float alpha  = ((materialType & 0x40) == 0) ? data.diffuseAndAlpha.a : inColor.a;
	#else
		vec3 ambient = data.ambientAndMaterialType.rgb;
		vec3 diffuse = data.diffuseAndAlpha.rgb;
		float alpha  = data.diffuseAndAlpha.a;
	#endif

		outColor = vec4(data.emission + (scene.ambientLight * ambient) +
		                (ambientProduct * ambient) +
		                (diffuseProduct * diffuse) +
		                (specularProduct * data.specular),
		                alpha);


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
