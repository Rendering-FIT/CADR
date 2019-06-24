#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in smooth vec3 eyePosition;
layout(location=1) in smooth vec3 eyeNormal;

layout(binding=0,std140) uniform Material {
	vec4 ambientColor;  // alpha not used
	vec4 diffuseColor;  // alpha used for transparency
};
layout(binding=1) uniform Global {
	vec3 globalAmbientLight;
};
layout(binding=2) uniform Light {
	vec4 lightData[3]; // position on offset 0, attenuation on offset 12, ambient on offset 24, diffuse on offset 36
};

layout(location=0) out vec4 fragColor;


void main() {

	// l - vertex to light direction, in eye coordinates
	vec3 l=lightData[0].xyz-eyePosition;
	float lLen=length(l);
	l/=lLen;

	// attenuation
	float attenuation=1./(lightData[0].w+lightData[1].x*lLen+lightData[1].y*lLen*lLen);

	// n - Normal of vertex, in eye coordinates
	vec3 n=normalize(eyeNormal);

	// invert normals on back facing triangles
	if(gl_FrontFacing==false)
		n=-n;

	// unpack data
	vec3 ambientLight=vec3(lightData[1].zw,lightData[2].x);
	vec3 diffuseLight=lightData[2].yzw;

	// Lambert's diffuse reflection
	float nDotL=dot(n,l);
	fragColor.rgb=(ambientLight*ambientColor.rgb+
						diffuseLight*diffuseColor.rgb*max(nDotL,0.))*attenuation;
	fragColor.a=diffuseColor.a;

	// global ambient
	fragColor.rgb+=globalAmbientLight*ambientColor.rgb;

}
