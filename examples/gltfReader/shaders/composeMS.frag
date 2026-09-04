// SPDX-FileCopyrightText: 2026 PCJohn (Jan Pečiva, peciva@fit.vut.cz)
//
// SPDX-License-Identifier: MIT-0

#version 460

// number of samples for multisampling
layout(constant_id = 0) const int numSamples = 4;

// input attachments
layout(input_attachment_index=0, set=0, binding=0) uniform subpassInputMS opaqueColorImage;
layout(input_attachment_index=1, set=0, binding=1) uniform subpassInputMS transparencyColorImage;
layout(input_attachment_index=2, set=0, binding=2) uniform subpassInputMS transparencyCountImage;

// output color
layout(location = 2) out vec4 outColor;


void main()
{
	// opaque color (average of all samples)
	outColor = subpassLoad(opaqueColorImage, 0);
	for(int i=1; i<numSamples; i++)
		outColor += subpassLoad(opaqueColorImage, i);
	outColor /= numSamples;

	// transparent color (sum of color and count for all samples)
	float transparencyCount = 0;
	vec4 transparencyColor = vec4(0);
	for(int i=0; i<numSamples; i++)
	{
		// transparency count and color
		float tc = subpassLoad(transparencyCountImage, i).r;
		if(tc != 0) {
			transparencyCount += tc;
			transparencyColor += subpassLoad(transparencyColorImage, i);
		}
	}

	// any transparency?
	if(transparencyColor.a != 0)
	{
		// compute final color using weighted average
		vec3  avgColor = (transparencyColor.rgb) / transparencyColor.a;
		float avgAlpha = transparencyColor.a / transparencyCount;
		float t = pow(1 - avgAlpha, transparencyCount);
		outColor.rgb = mix(avgColor, outColor.rgb, t);
	}
}
