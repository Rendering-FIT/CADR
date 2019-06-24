#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in smooth vec3 eyePosition;
layout(location=1) in smooth vec3 eyeNormal;
layout(location=2) in smooth vec4 color;
layout(location=3) in smooth vec2 texCoord;

layout(binding=0,std140) uniform Material {
	vec4 ambientColor;
	vec4 diffuseColor;
	vec4 specularColor;
	vec4 emissiveColor;
	float shininess;
	int textureMode; // 0 - no texturing, 0x2100 - modulate, 0x1e01 - replace, 0x2101 - decal*/
};
layout(binding=1) uniform Global {
	vec3 globalAmbientLight;
};
layout(binding=2) uniform Light {
	vec4 lightPosition;
	vec4 lightAttenuation;
	vec4 ambientLight;
	vec4 diffuseLight;
	vec4 specularLight;
};
layout(binding=3) uniform sampler2D textureSampler;

layout(location=0) out vec4 fragColor;


void main() {

	// read texture
	vec4 textureColor;
	if(textureMode!=0)
		textureColor=texture(textureSampler,texCoord);

	// surface color
	if(textureMode==0x1e01)  // if GL_REPLACE

		// apply GL_REPLACE texture
		fragColor=vec4(textureColor.rgb,textureColor.a*color.a);

	else {

		// l - vertex to light direction, in eye coordinates
		vec3 l=lightPosition.xyz;
		if(lightPosition.w!=0.) {
			if(lightPosition.w!=1.)
				l/=lightPosition.w;
			l-=eyePosition;
		}
		float lLen=length(l);
		l/=lLen;

		// attenuation
		float attenuation=1.;
		if(lightPosition.w!=0.)
			attenuation/=lightAttenuation.x+lightAttenuation.y*lLen+lightAttenuation.z*lLen*lLen;

		// n - Normal of vertex, in eye coordinates
		vec3 n=normalize(eyeNormal);

		// invert normals on back facing triangles
		if(gl_FrontFacing==false)
			n=-n;

		// Lambert's diffuse reflection
		float nDotL=dot(n,l);
		fragColor.rgb=(ambientLight.rgb*ambientColor.rgb+
		               diffuseLight.rgb*diffuseColor.rgb*max(nDotL,0.))*attenuation;
		fragColor.a=color.a;

		// global ambient and emissive color
		fragColor.rgb+=globalAmbientLight*ambientColor.rgb+emissiveColor.rgb;

		// apply texture
		if(textureMode!=0) {

			if(textureMode==0x2100) // GL_MODULATE
				fragColor=fragColor*textureColor;
			else if(textureMode==0x2101)  // GL_DECAL
				fragColor=vec4(fragColor.rgb*(1-textureColor.a)+textureColor.rgb*textureColor.a,fragColor.a);

		}

		// specular effect (it is applied after the texture)
		if(nDotL>0.) {
			vec3 r=reflect(-l,n);
			float rDotV=dot(r,-normalize(eyePosition));
			if(rDotV>0.)
				fragColor.rgb+=specularLight.rgb*specularColor.rgb*pow(rDotV,shininess)*attenuation;
		}

	}

}
