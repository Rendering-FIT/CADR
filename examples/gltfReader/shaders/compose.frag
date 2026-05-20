#version 460

layout(input_attachment_index=0, set=0, binding=0) uniform subpassInputMS opaqueColorImage;
layout(input_attachment_index=1, set=0, binding=1) uniform subpassInputMS transparencyColorImage;
layout(input_attachment_index=2, set=0, binding=2) uniform subpassInputMS transparencyCountImage;

layout(location = 2) out vec4 outColor;


void main()
{
	// opaque color
	outColor = subpassLoad(opaqueColorImage, gl_SampleID);

	// any transparent objects?
	float transparencyCount = subpassLoad(transparencyCountImage, gl_SampleID).r;
	if(transparencyCount != 0) {

		// transparency color
		vec4 transparencyColor = subpassLoad(transparencyColorImage, gl_SampleID);

		// any transparency?
		if(transparencyColor.a != 0) {

			// compute final color using weighted average
			vec3  avgColor = (transparencyColor.rgb) / transparencyColor.a;
			float avgAlpha = transparencyColor.a / transparencyCount;
			float t = pow(1 - avgAlpha, transparencyCount);
			outColor.rgb = mix(avgColor, outColor.rgb, t);

		}
	}
}
