#version 460


// output to geometry shader
layout(location = 0) out flat int outDrawIndex;
layout(location = 1) out flat int outInstanceIndex;
layout(location = 2) out flat int outVertexIndex;
#ifdef ID_BUFFER
layout(location = 3) out flat uvec2 outId;
#endif



void main()
{
	outDrawIndex = gl_DrawID;
	outInstanceIndex = gl_InstanceIndex;
	outVertexIndex = gl_VertexIndex;
#ifdef ID_BUFFER
	outId[0] = gl_DrawID;
	outId[1] = gl_InstanceIndex;
#endif
}
