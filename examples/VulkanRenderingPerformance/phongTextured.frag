#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in smooth vec3 eyePosition;
layout(location=1) in smooth vec3 eyeNormal;
layout(location=2) in smooth vec4 color;
layout(location=3) in smooth vec2 texCoord;

layout(binding=0,std140) uniform Material {
	vec4 materialData[3]; // ambient on offset 0, diffuse on offset 12, specular on offset 24, emissive on offset 36
	layout(offset=48) float shininess;
	layout(offset=52) int textureMode; // 0 - no texturing, 0x2100 - modulate, 0x1e01 - replace, 0x2101 - decal*/
};
layout(binding=1) uniform Global {
	vec3 globalAmbientLight;
};
layout(binding=2) uniform Light {
	vec4 lightPosition;
	vec4 lightData[3]; // attenuation on offset 16, ambient on offset 28, diffuse on offset 40, specular on offset 52
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
			attenuation/=lightData[0].x+lightData[0].y*lLen+lightData[0].z*lLen*lLen;

		// n - Normal of vertex, in eye coordinates
		vec3 n=normalize(eyeNormal);

		// invert normals on back facing triangles
		if(gl_FrontFacing==false)
			n=-n;

		// unpack material data
		vec3 ambientColor=vec3(materialData[0].rgb);
		vec3 diffuseColor=vec3(materialData[0].a,materialData[1].rg);
		vec3 specularColor=vec3(materialData[1].ba,materialData[2].r);
		vec3 emissiveColor=vec3(materialData[2].gba);

		// unpack light data
		vec3 ambientLight=vec3(lightData[0].a,lightData[1].rg);
		vec3 diffuseLight=vec3(lightData[1].ba,lightData[2].r);
		vec3 specularLight=vec3(lightData[2].gba);

		// Lambert's diffuse reflection
		float nDotL=dot(n,l);
		fragColor.rgb=(ambientLight*ambientColor+
		               diffuseLight*diffuseColor*max(nDotL,0.))*attenuation;
		fragColor.a=color.a;

		// global ambient and emissive color
		fragColor.rgb+=globalAmbientLight*ambientColor+emissiveColor;

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
				fragColor.rgb+=specularLight*specularColor*pow(rDotV,shininess)*attenuation;
		}

	}

}
