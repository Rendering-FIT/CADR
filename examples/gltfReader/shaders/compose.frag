// SPDX-FileCopyrightText: 2026 PCJohn (Jan Pečiva, peciva@fit.vut.cz)
//
// SPDX-License-Identifier: MIT-0

#version 460

layout(input_attachment_index=0, set=0, binding=0) uniform subpassInput opaqueColorImage;
layout(input_attachment_index=1, set=0, binding=1) uniform subpassInput transparencyColorImage;
layout(input_attachment_index=2, set=0, binding=2) uniform subpassInput transparencyCountImage;

layout(location = 2) out vec4 outColor;


void main()
{
	// opaque color
	outColor = subpassLoad(opaqueColorImage);

	// any transparent objects?
	float transparencyCount = subpassLoad(transparencyCountImage).r;
	if(transparencyCount != 0) {

		// transparency color
		vec4 transparencyColor = subpassLoad(transparencyColorImage);

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
