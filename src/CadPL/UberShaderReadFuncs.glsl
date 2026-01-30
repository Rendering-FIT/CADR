
//
//  buffer references
//

layout(buffer_reference, std430, buffer_reference_align=16) restrict readonly buffer
AlignedVec4Ref {
	vec4 value;
};

layout(buffer_reference, std430, buffer_reference_align=16) restrict readonly buffer
AlignedUVec4Ref {
	uvec4 value;
};

layout(buffer_reference, std430, buffer_reference_align=16) restrict readonly buffer
AlignedIVec4Ref {
	ivec4 value;
};

layout(buffer_reference, std430, buffer_reference_align=16) restrict readonly buffer
AlignedVec3Ref {
	vec3 value;
};

layout(buffer_reference, std430, buffer_reference_align=16) restrict readonly buffer
AlignedUVec3Ref {
	uvec3 value;
};

layout(buffer_reference, std430, buffer_reference_align=16) restrict readonly buffer
AlignedIVec3Ref {
	ivec3 value;
};

layout(buffer_reference, std430, buffer_reference_align=4) restrict readonly buffer
UnalignedVec3Ref {
	vec3 value;
};

layout(buffer_reference, std430, buffer_reference_align=4) restrict readonly buffer
UnalignedUVec3Ref {
	uvec3 value;
};

layout(buffer_reference, std430, buffer_reference_align=4) restrict readonly buffer
UnalignedIVec3Ref {
	ivec3 value;
};

layout(buffer_reference, std430, buffer_reference_align=8) restrict readonly buffer
AlignedVec2Ref {
	vec2 value;
};

layout(buffer_reference, std430, buffer_reference_align=8) restrict readonly buffer
AlignedUVec2Ref {
	uvec2 value;
};

layout(buffer_reference, std430, buffer_reference_align=8) restrict readonly buffer
AlignedIVec2Ref {
	ivec2 value;
};

layout(buffer_reference, std430, buffer_reference_align=4) restrict readonly buffer
UnalignedVec2Ref {
	vec2 value;
};

layout(buffer_reference, std430, buffer_reference_align=4) restrict readonly buffer
UnalignedUVec2Ref {
	uvec2 value;
};

layout(buffer_reference, std430, buffer_reference_align=4) restrict readonly buffer
UnalignedIVec2Ref {
	ivec2 value;
};

layout(buffer_reference, std430, buffer_reference_align=4) restrict readonly buffer
AlignedFloatRef {
	float value;
};

layout(buffer_reference, std430, buffer_reference_align=4) restrict readonly buffer
AlignedUIntRef {
	uint value;
};

layout(buffer_reference, std430, buffer_reference_align=4) restrict readonly buffer
AlignedIntRef {
	int value;
};



//
//  read float
//
float readFloat(uint attribAccessInfo, uint64_t vertexDataPtr)
{
	// settings:
	// bits 0..7 - offset (0..255)
	// bits 8..15 - type
	//
	// type values:
	//   - 0x80 - float, alignment 4
	//   - 0x81 - half, alignment 4
	//   - 0x82 - half, alignment 4, reads the values with additional offset +2
	//   - 0x83 - uint, alignment 4, normalize
	//   - 0x84 - uint, alignment 4
	//   - 0x85 - int2, alignment 4, normalize
	//   - 0x86 - int2, alignment 4
	//   - 0x87 - ushort, alignment 4, normalize
	//   - 0x88 - ushort, alignment 4
	//   - 0x89 - ushort, alignment 4, reads the values with additional offset +2, normalize
	//   - 0x8a - ushort, alignment 4, reads the values with additional offset +2
	//   - 0x8b - short, alignment 4, normalize
	//   - 0x8c - short, alignment 4
	//   - 0x8d - short, alignment 4, reads the values with additional offset +2, normalize
	//   - 0x8e - short, alignment 4, reads the values with additional offset +2
	//   - 0x8f - ubyte, alignment 4, normalize
	//   - 0x90 - ubyte, alignment 4
	//   - 0x91 - ubyte, alignment 4, reads the values with additional offset +1, normalize
	//   - 0x92 - ubyte, alignment 4, reads the values with additional offset +1
	//   - 0x93 - ubyte, alignment 4, reads the values with additional offset +2, normalize
	//   - 0x94 - ubyte, alignment 4, reads the values with additional offset +2
	//   - 0x95 - ubyte, alignment 4, reads the values with additional offset +3, normalize
	//   - 0x96 - ubyte, alignment 4, reads the values with additional offset +3
	//   - 0x97 - byte, alignment 4, normalize
	//   - 0x98 - byte, alignment 4
	//   - 0x99 - byte, alignment 4, reads the values with additional offset +1, normalize
	//   - 0x9a - byte, alignment 4, reads the values with additional offset +1
	//   - 0x9b - byte, alignment 4, reads the values with additional offset +2, normalize
	//   - 0x9c - byte, alignment 4, reads the values with additional offset +2
	//   - 0x9d - byte, alignment 4, reads the values with additional offset +3, normalize
	//   - 0x9e - byte, alignment 4, reads the values with additional offset +3

	uint offset = attribAccessInfo & 0x00ff;  // max offset is 255
	uint64_t addr = vertexDataPtr + offset;
	uint type = attribAccessInfo & 0xff00;

	// float
	if(type == 0x8000)
		return AlignedFloatRef(addr).value;

	// half
	if(type == 0x8100) {
		uint v = AlignedUIntRef(addr).value;
		return unpackHalf2x16(v).x;
	}
	if(type == 0x8200) {
		uint v = AlignedUIntRef(addr).value;
		return unpackHalf2x16(v).y;
	}

	// uint
	if(type == 0x8300) {
		// alignment 4, normalize
		uint v = AlignedUIntRef(addr).value;
		return float(v) / 0xffffffff;
	}
	if(type == 0x8400)
		// alignment 4, do not normalize
		return AlignedUIntRef(addr).value;

	// int
	if(type == 0x8500) {
		// alignment 4, normalize
		int v = AlignedIntRef(addr).value;
		return max(float(v) / 0x7fffffff, -1.);
	}
	if(type == 0x8600)
		// alignment 4, do not normalize
		return AlignedIntRef(addr).value;

	// ushort
	if(type == 0x8700) {
		// alignment 4, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackUnorm2x16(v).x;
	}
	if(type == 0x8800) {
		// alignment 4, do not normalize
		uint v = AlignedUIntRef(addr).value;
		return float(v & 0xffff);
	}
	if(type == 0x8900) {
		// alignment 4, offset +2, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackUnorm2x16(v).y;
	}
	if(type == 0x8a00) {
		// alignment 4, offset +2, do not normalize
		uint v = AlignedUIntRef(addr).value;
		return float(v >> 16);
	}

	// short
	if(type == 0x8b00) {
		// alignment 4, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackSnorm2x16(v).x;
	}
	if(type == 0x8c00) {
		// alignment 4, do not normalize
		uint v = AlignedUIntRef(addr).value;
		int r = int(v & 0xffff);
		r |= 0xffff0000 * (r >> 15);
		return r;
	}
	if(type == 0x8d00) {
		// alignment 4, offset +2, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackSnorm2x16(v).y;
	}
	if(type == 0x8e00) {
		// alignment 4, offset +2, do not normalize
		uint v = AlignedUIntRef(addr).value;
		int r = int(v >> 16);
		r |= 0xffff0000 * (r >> 15);
		return r;
	}

	// ubyte
	if(type == 0x8f00) {
		// alignment 4, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackUnorm4x8(v).x;
	}
	if(type == 0x9000) {
		// alignment 4, do not normalize
		uint v = AlignedUIntRef(addr).value;
		return float(v & 0xff);
	}
	if(type == 0x9100) {
		// alignment 4, offset +1, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackUnorm4x8(v).y;
	}
	if(type == 0x9200) {
		// alignment 4, offset +1 do not normalize
		uint v = AlignedUIntRef(addr).value;
		return float((v >> 8) & 0xff);
	}
	if(type == 0x9300) {
		// alignment 4, offset +2, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackUnorm4x8(v).z;
	}
	if(type == 0x9400) {
		// alignment 4, offset +2 do not normalize
		uint v = AlignedUIntRef(addr).value;
		return float((v >> 16) & 0xff);
	}
	if(type == 0x9500) {
		// alignment 4, offset +3, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackUnorm4x8(v).w;
	}
	if(type == 0x9600) {
		// alignment 4, offset +3, do not normalize
		uint v = AlignedUIntRef(addr).value;
		return float(v >> 24);
	}

	// byte
	if(type == 0x9700) {
		// alignment 4, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackSnorm4x8(v).x;
	}
	if(type == 0x9800) {
		// alignment 4, do not normalize
		uint v = AlignedUIntRef(addr).value;
		int r = int(v & 0xff);
		r |= 0xffffff00 * (r >> 7);
		return float(r);
	}
	if(type == 0x9900) {
		// alignment 4, offset +1, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackUnorm4x8(v).y;
	}
	if(type == 0x9a00) {
		// alignment 4, offset +1 do not normalize
		uint v = AlignedUIntRef(addr).value;
		int r = int((v >> 8) & 0xff);
		r |= 0xffffff00 * (r >> 7);
		return float(r);
	}
	if(type == 0x9b00) {
		// alignment 4, offset +2, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackUnorm4x8(v).z;
	}
	if(type == 0x9c00) {
		// alignment 4, offset +2 do not normalize
		uint v = AlignedUIntRef(addr).value;
		int r = int((v >> 16) & 0xff);
		r |= 0xffffff00 * (r >> 7);
		return float(r);
	}
	if(type == 0x9d00) {
		// alignment 4, offset +3, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackUnorm4x8(v).w;
	}
	if(type == 0x9e00) {
		// alignment 4, offset +3, do not normalize
		uint v = AlignedUIntRef(addr).value;
		return float(v >> 24);
	}

	// return NaN
	return float(0/0);
}



//
//  read vec2
//
vec2 readVec2(uint attribAccessInfo, uint64_t vertexDataPtr)
{
	// settings:
	// bits 0..7 - offset (0..255)
	// bits 8..15 - type
	//
	// type values:
	//   - 0x50 - float2, alignment 8
	//   - 0x51 - float2, alignment 4
	//   - 0x52 - half2, alignment 4
	//   - 0x53 - half2, alignment 4, reads the values with additional offset +2
	//   - 0x54 - uint2, alignment 8, normalize
	//   - 0x55 - uint2, alignment 8
	//   - 0x56 - uint2, alignment 4, normalize
	//   - 0x57 - uint2, alignment 4
	//   - 0x58 - int2, alignment 8, normalize
	//   - 0x58 - int2, alignment 8
	//   - 0x5a - int2, alignment 4, normalize
	//   - 0x5b - int2, alignment 4
	//   - 0x5c - ushort2, alignment 4, normalize
	//   - 0x5d - ushort2, alignment 4
	//   - 0x5e - ushort2, alignment 4, reads the values with additional offset +2, normalize
	//   - 0x5f - ushort2, alignment 4, reads the values with additional offset +2
	//   - 0x60 - short2, alignment 4, normalize
	//   - 0x61 - short2, alignment 4
	//   - 0x62 - short2, alignment 4, reads the values with additional offset +2, normalize
	//   - 0x63 - short2, alignment 4, reads the values with additional offset +2
	//   - 0x64 - ubyte2, alignment 4, normalize
	//   - 0x65 - ubyte2, alignment 4
	//   - 0x66 - ubyte2, alignment 4, reads the values with additional offset +1, normalize
	//   - 0x67 - ubyte2, alignment 4, reads the values with additional offset +1
	//   - 0x68 - ubyte2, alignment 4, reads the values with additional offset +2, normalize
	//   - 0x69 - ubyte2, alignment 4, reads the values with additional offset +2
	//   - 0x6a - ubyte2, alignment 4, reads the values with additional offset +3, normalize
	//   - 0x6b - ubyte2, alignment 4, reads the values with additional offset +3
	//   - 0x6c - byte2, alignment 4, normalize
	//   - 0x6d - byte2, alignment 4
	//   - 0x6e - byte2, alignment 4, reads the values with additional offset +1, normalize
	//   - 0x6f - byte2, alignment 4, reads the values with additional offset +1
	//   - 0x70 - byte2, alignment 4, reads the values with additional offset +2, normalize
	//   - 0x71 - byte2, alignment 4, reads the values with additional offset +2
	//   - 0x72 - byte2, alignment 4, reads the values with additional offset +3, normalize
	//   - 0x73 - byte2, alignment 4, reads the values with additional offset +3

	uint offset = attribAccessInfo & 0x00ff;  // max offset is 255
	uint64_t addr = vertexDataPtr + offset;
	uint type = attribAccessInfo & 0xff00;

	// float2
	if(type == 0x5000)
		return AlignedVec2Ref(addr).value;
	if(type == 0x5100)
		return UnalignedVec2Ref(addr).value;

	// half2
	if(type == 0x5200) {
		uint v = AlignedUIntRef(addr).value;
		return unpackHalf2x16(v);
	}
	if(type == 0x5300) {
		uvec2 v = UnalignedUVec2Ref(addr).value;
		v[0] = (v[0] >> 16) | (v[1] << 16);
		return unpackHalf2x16(v[0]);
	}

	// uint2
	if(type == 0x5400) {
		// alignment 8, normalize
		uvec2 v = AlignedUVec2Ref(addr).value;
		return vec2(float(v.x) / 0xffffffff, float(v.y) / 0xffffffff);
	}
	if(type == 0x5500)
		// alignment 8, do not normalize
		return AlignedUVec2Ref(addr).value;
	if(type == 0x5600) {
		// alignment 4, normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		return vec2(float(v.x) / 0xffffffff, float(v.y) / 0xffffffff);
	}
	if(type == 0x5700)
		// alignment 4, do not normalize
		return UnalignedUVec2Ref(addr).value;

	// int2
	if(type == 0x5800) {
		// alignment 8, normalize
		ivec2 v = AlignedIVec2Ref(addr).value;
		return max(vec2(float(v.x) / 0x7fffffff, float(v.y) / 0x7fffffff),
		           -1.);
	}
	if(type == 0x5900)
		// alignment 8, do not normalize
		return AlignedIVec2Ref(addr).value;
	if(type == 0x5a00) {
		// alignment 4, normalize
		ivec2 v = UnalignedIVec2Ref(addr).value;
		return max(vec2(float(v.x) / 0x7fffffff, float(v.y) / 0x7fffffff),
		           -1.);
	}
	if(type == 0x5b00)
		// alignment 4, do not normalize
		return UnalignedIVec2Ref(addr).value;

	// ushort2
	if(type == 0x5c00) {
		// alignment 4, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackUnorm2x16(v);
	}
	if(type == 0x5d00) {
		// alignment 4, do not normalize
		uint v = AlignedUIntRef(addr).value;
		return vec2(v & 0xffff, v >> 16);
	}
	if(type == 0x5e00) {
		// alignment 4, offset +2, normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		v[0] = (v[0] >> 16) | (v[1] << 16);
		return unpackUnorm2x16(v[0]);
	}
	if(type == 0x5f00) {
		// alignment 4, offset +2, do not normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		return vec2(v[0] >> 16, v[1] & 0xffff);
	}

	// short2
	if(type == 0x6000) {
		// alignment 4, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackSnorm2x16(v);
	}
	if(type == 0x6100) {
		// alignment 4, do not normalize
		uint v = AlignedUIntRef(addr).value;
		ivec2 r = ivec2(int(v & 0xffff), int(v >> 16));
		r |= 0xffff0000 * (r >> 15);
		return vec2(r);
	}
	if(type == 0x6200) {
		// alignment 4, offset +2, normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		return unpackSnorm2x16(v[0]);
	}
	if(type == 0x6300) {
		// alignment 4, offset +2, do not normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		ivec2 r = ivec2(int(v[0] >> 16), int(v[1] & 0xffff));
		r |= 0xffff0000 * (r >> 15);
		return vec2(r);
	}

	// ubyte
	if(type == 0x6400) {
		// alignment 4, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackUnorm4x8(v).xy;
	}
	if(type == 0x6500) {
		// alignment 4, do not normalize
		uint v = AlignedUIntRef(addr).value;
		return vec2(v & 0xff, (v >> 8) & 0xff);
	}
	if(type == 0x6600) {
		// alignment 4, offset +1, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackUnorm4x8(v).yz;
	}
	if(type == 0x6700) {
		// alignment 4, offset +1 do not normalize
		uint v = AlignedUIntRef(addr).value;
		return vec2((v >> 8) & 0xff, (v >> 16) & 0xff);
	}
	if(type == 0x6800) {
		// alignment 4, offset +2, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackUnorm4x8(v).zw;
	}
	if(type == 0x6900) {
		// alignment 4, offset +2 do not normalize
		uint v = AlignedUIntRef(addr).value;
		return vec2((v >> 16) & 0xff, (v >> 24) & 0xff);
	}
	if(type == 0x6a00) {
		// alignment 4, offset +3, normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		v[0] = (v[0] >> 24) | ((v[1] & 0xff) << 8);
		return unpackUnorm4x8(v[0]).xy;
	}
	if(type == 0x6b00) {
		// alignment 4, offset +3, do not normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		return vec2(v[0] >> 24, v[1] & 0xff);
	}

	// byte
	if(type == 0x6c00) {
		// alignment 4, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackSnorm4x8(v).xy;
	}
	if(type == 0x6d00) {
		// alignment 4, do not normalize
		uint v = AlignedUIntRef(addr).value;
		ivec2 r = ivec2(int(v & 0xff), int((v >> 8) & 0xff));
		r |= 0xffffff00 * (r >> 7);
		return vec2(r);
	}
	if(type == 0x6e00) {
		// alignment 4, offset +1, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackUnorm4x8(v).yz;
	}
	if(type == 0x6f00) {
		// alignment 4, offset +1 do not normalize
		uint v = AlignedUIntRef(addr).value;
		ivec2 r = ivec2(int((v >> 8) & 0xff), int((v >> 16) & 0xff));
		r |= 0xffffff00 * (r >> 7);
		return vec2(r);
	}
	if(type == 0x7000) {
		// alignment 4, offset +2, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackUnorm4x8(v).zw;
	}
	if(type == 0x7100) {
		// alignment 4, offset +2 do not normalize
		uint v = AlignedUIntRef(addr).value;
		ivec2 r = ivec2(int((v >> 16) & 0xff), int((v >> 24) & 0xff));
		r |= 0xffffff00 * (r >> 7);
		return vec2(r);
	}
	if(type == 0x7200) {
		// alignment 4, offset +3, normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		v[0] = (v[0] >> 24) | ((v[1] & 0xff) << 8);
		return unpackUnorm4x8(v[0]).xy;
	}
	if(type == 0x7300) {
		// alignment 4, offset +3, do not normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		return vec2(v[0] >> 24, (v[1] & 0xff) << 8);
	}

	// return NaN
	return vec2(0/0);
}



//
//  read vec3
//
vec3 readVec3(uint attribAccessInfo, uint64_t vertexDataPtr)
{
	// settings:
	// bits 0..7 - offset (0..255)
	// bits 8..15 - type
	//
	// type values:
	//   - 0x20 - float3, alignment 16
	//   - 0x21 - float3, alignment 4
	//   - 0x22 - half3, alignment 4, on 8 bytes reads first six bytes
	//   - 0x23 - half3, alignment 4, on 8 bytes reads last six bytes
	//   - 0x24 - uint3, alignment 16, normalize
	//   - 0x25 - uint3, alignment 16
	//   - 0x26 - uint3, alignment 4, normalize
	//   - 0x27 - uint3, alignment 4
	//   - 0x28 - int3, alignment 16, normalize
	//   - 0x29 - int3, alignment 16
	//   - 0x2a - int3, alignment 4, normalize
	//   - 0x2b - int3, alignment 4
	//   - 0x2c - ushort3, alignment 4, on 8 bytes reads first six bytes, normalize
	//   - 0x2d - ushort3, alignment 4, on 8 bytes reads first six bytes
	//   - 0x2e - ushort3, alignment 4, on 8 bytes reads last six bytes, normalize
	//   - 0x2f - ushort3, alignment 4, on 8 bytes reads last six bytes
	//   - 0x30 - short3, alignment 4, on 8 bytes reads first six bytes, normalize
	//   - 0x31 - short3, alignment 4, on 8 bytes reads first six bytes
	//   - 0x32 - short3, alignment 4, on 8 bytes reads last six bytes, normalize
	//   - 0x33 - short3, alignment 4, on 8 bytes reads last six bytes
	//   - 0x34 - ubyte3, alignment 4, on 4 bytes extracts first three bytes, normalize
	//   - 0x35 - ubyte3, alignment 4, on 4 bytes extracts first three bytes
	//   - 0x36 - ubyte3, alignment 4, on 4 bytes extracts last three bytes, normalize
	//   - 0x37 - ubyte3, alignment 4, on 4 bytes extracts last three bytes
	//   - 0x38 - ubyte3, alignment 4, on 4 bytes extracts first three bytes, reads the values with additional offset +2, normalize
	//   - 0x39 - ubyte3, alignment 4, on 4 bytes extracts first three bytes, reads the values with additional offset +2
	//   - 0x3a - ubyte3, alignment 4, on 4 bytes extracts last three bytes, reads the values with additional offset +2, normalize
	//   - 0x3b - ubyte3, alignment 4, on 4 bytes extracts last three bytes, reads the values with additional offset +2
	//   - 0x3c - byte3, alignment 4, on 4 bytes extracts first three bytes, normalize
	//   - 0x3d - byte3, alignment 4, on 4 bytes extracts first three bytes
	//   - 0x3e - byte3, alignment 4, on 4 bytes extracts last three bytes, normalize
	//   - 0x3f - byte3, alignment 4, on 4 bytes extracts last three bytes
	//   - 0x40 - byte3, alignment 4, on 4 bytes extracts first three bytes, reads the values with additional offset +2, normalize
	//   - 0x41 - byte3, alignment 4, on 4 bytes extracts first three bytes, reads the values with additional offset +2
	//   - 0x42 - byte3, alignment 4, on 4 bytes extracts last three bytes, reads the values with additional offset +2, normalize
	//   - 0x43 - byte3, alignment 4, on 4 bytes extracts last three bytes, reads the values with additional offset +2

	uint offset = attribAccessInfo & 0x00ff;  // max offset is 255
	uint64_t addr = vertexDataPtr + offset;
	uint type = attribAccessInfo & 0xff00;

	// float3
	if(type == 0x2000)
		return AlignedVec3Ref(addr).value;
	if(type == 0x2100)
		return UnalignedVec3Ref(addr).value;

	// half3
	if(type == 0x2200) {
		uvec2 v = UnalignedUVec2Ref(addr).value;
		return vec3(unpackHalf2x16(v[0]), unpackHalf2x16(v[1]).x);
	}
	if(type == 0x2300) {
		uvec2 v = UnalignedUVec2Ref(addr).value;
		return vec3(unpackHalf2x16(v[0]).y, unpackHalf2x16(v[1]));
	}

	// uint3
	if(type == 0x2400) {
		// alignment 16, normalize
		uvec3 v = AlignedUVec3Ref(addr).value;
		return vec3(float(v.x) / 0xffffffff, float(v.y) / 0xffffffff,
		            float(v.z) / 0xffffffff);
	}
	if(type == 0x2500)
		// alignment 16, do not normalize
		return AlignedUVec3Ref(addr).value;
	if(type == 0x2600) {
		// alignment 4, normalize
		uvec3 v = UnalignedUVec3Ref(addr).value;
		return vec3(float(v.x) / 0xffffffff, float(v.y) / 0xffffffff,
		            float(v.z) / 0xffffffff);
	}
	if(type == 0x2700)
		// alignment 16, do not normalize
		return UnalignedUVec3Ref(addr).value;

	// int3
	if(type == 0x2800) {
		// alignment 16, normalize
		ivec3 v = AlignedIVec3Ref(addr).value;
		return max(vec3(float(v.x) / 0x7fffffff, float(v.y) / 0x7fffffff,
		                float(v.z) / 0x7fffffff),
		           -1.);
	}
	if(type == 0x2900)
		// alignment 16, do not normalize
		return AlignedIVec3Ref(addr).value;
	if(type == 0x2a00) {
		// alignment 4, normalize
		ivec3 v = UnalignedIVec3Ref(addr).value;
		return max(vec3(float(v.x) / 0x7fffffff, float(v.y) / 0x7fffffff,
		                float(v.z) / 0x7fffffff),
		           -1.);
	}
	if(type == 0x2b00)
		// alignment 4, do not normalize
		return UnalignedIVec3Ref(addr).value;

	// ushort3
	if(type == 0x2c00) {
		// alignment 4, normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		return vec3(unpackUnorm2x16(v[0]), unpackUnorm2x16(v[1]).x);
	}
	if(type == 0x2d00) {
		// alignment 4, do not normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		return vec3(v[0] & 0xffff, v[0] >> 16, v[1] & 0xffff);
	}
	if(type == 0x2e00) {
		// alignment 4, offset +2, normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		return vec3(unpackUnorm2x16(v[0]).y, unpackUnorm2x16(v[1]));
	}
	if(type == 0x2f00) {
		// alignment 4, offset +2, do not normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		return vec3(v[0] >> 16, v[1] & 0xffff, v[1] >> 16);
	}

	// short3
	if(type == 0x3000) {
		// alignment 4, normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		return vec3(unpackSnorm2x16(v[0]), unpackSnorm2x16(v[1]).x);
	}
	if(type == 0x3100) {
		// alignment 4, do not normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		ivec3 r = ivec3(int(v[0] & 0xffff), int(v[0] >> 16), int(v[1] & 0xffff));
		r |= 0xffff0000 * (r >> 15);
		return vec3(r);
	}
	if(type == 0x3200) {
		// alignment 4, offset +2, normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		return vec3(unpackSnorm2x16(v[0]).y, unpackSnorm2x16(v[1]));
	}
	if(type == 0x3300) {
		// alignment 4, offset +2, do not normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		ivec3 r = ivec3(int(v[0] >> 16), int(v[1] & 0xffff), int(v[1] >> 16));
		r |= 0xffff0000 * (r >> 15);
		return vec3(r);
	}

	// ubyte
	if(type == 0x3400) {
		// alignment 4, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackUnorm4x8(v).xyz;
	}
	if(type == 0x3500) {
		// alignment 4, do not normalize
		uint v = AlignedUIntRef(addr).value;
		return vec3(v & 0xff, (v >> 8) & 0xff, (v >> 16) & 0xff);
	}
	if(type == 0x3600) {
		// alignment 4, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackUnorm4x8(v).yzw;
	}
	if(type == 0x3700) {
		// alignment 4, do not normalize
		uint v = AlignedUIntRef(addr).value;
		return vec3((v >> 8) & 0xff, (v >> 16) & 0xff, (v >> 24) & 0xff);
	}
	if(type == 0x3800) {
		// alignment 4, offset +2, normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		v[0] = (v[0] >> 16) | (v[1] << 16);
		return unpackUnorm4x8(v[0]).xyz;
	}
	if(type == 0x3900) {
		// alignment 4, offset +2, do not normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		v[0] = (v[0] >> 16) | (v[1] << 16);
		return vec3(v[0] & 0xff, (v[0] >> 8) & 0xff, (v[0] >> 16) & 0xff);
	}
	if(type == 0x3a00) {
		// alignment 4, offset +2, normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		v[0] = (v[0] >> 16) | (v[1] << 16);
		return unpackUnorm4x8(v[0]).yzw;
	}
	if(type == 0x3b00) {
		// alignment 4, offset +2, do not normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		v[0] = (v[0] >> 16) | (v[1] << 16);
		return vec3((v[0] >> 8) & 0xff, (v[0] >> 16) & 0xff, (v[0] >> 24) & 0xff);
	}

	// byte
	if(type == 0x3c00) {
		// alignment 4, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackSnorm4x8(v).xyz;
	}
	if(type == 0x3d00) {
		// alignment 4, do not normalize
		uint v = AlignedUIntRef(addr).value;
		ivec3 r = ivec3(int(v & 0xff), int((v >> 8) & 0xff), int((v >> 16) & 0xff));
		r |= 0xffffff00 * (r >> 7);
		return vec3(r);
	}
	if(type == 0x3e00) {
		// alignment 4, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackSnorm4x8(v).yzw;
	}
	if(type == 0x3f00) {
		// alignment 4, do not normalize
		uint v = AlignedUIntRef(addr).value;
		ivec3 r = ivec3(int((v >> 8) & 0xff), int((v >> 16) & 0xff), int((v >> 24) & 0xff));
		r |= 0xffffff00 * (r >> 7);
		return vec3(r);
	}
	if(type == 0x4000) {
		// alignment 4, offset +2, normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		v[0] = (v[0] >> 16) | (v[1] << 16);
		return unpackSnorm4x8(v[0]).xyz;
	}
	if(type == 0x4100) {
		// alignment 4, offset +2, do not normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		v[0] = (v[0] >> 16) | (v[1] << 16);
		ivec3 r = ivec3(int(v[0] & 0xff), int((v[0] >> 8) & 0xff), int((v[0] >> 16) & 0xff));
		r |= 0xffffff00 * (r >> 7);
		return vec3(r);
	}
	if(type == 0x4200) {
		// alignment 4, offset +2, normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		v[0] = (v[0] >> 16) | (v[1] << 16);
		return unpackSnorm4x8(v[0]).yzw;
	}
	if(type == 0x4300) {
		// alignment 4, offset +2, do not normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		v[0] = (v[0] >> 16) | (v[1] << 16);
		ivec3 r = ivec3(int((v[0] >> 8) & 0xff), int((v[0] >> 16) & 0xff), int((v[0] >> 24) >> 0xff));
		r |= 0xffffff00 * (r >> 7);
		return vec3(r);
	}

	// return NaN
	return vec3(0/0);
}



//
//  read vec4
//
vec4 readVec4(uint attribAccessInfo, uint64_t vertexDataPtr)
{
	// settings:
	// bits 0..7 - offset (0..255)
	// bits 8..15 - type
	//
	// type values:
	//   - 0x00 - not used attribute
	//   - 0x01 - float4, alignment 16
	//   - 0x02 - half4, alignment 8
	//   - 0x03 - half4, alignment 4
	//   - 0x04 - half4, alignment 4, reads the values with additional offset +2
	//   - 0x05 - uint4 normalized, alignment 16
	//   - 0x06 - uint4, alignment 16
	//   - 0x07 - int4 normalized, alignment 16
	//   - 0x08 - int4, alignment 16
	//   - 0x09 - ushort4 normalized, alignment 8
	//   - 0x0a - ushort4, alignment 8
	//   - 0x0b - ushort4 normalized, alignment 4
	//   - 0x0c - ushort4, alignment 4
	//   - 0x0d - ushort4 normalized, alignment 4, reads the values with additional offset +2
	//   - 0x0e - ushort4, alignment 4, reads the values with additional offset +2
	//   - 0x0f - short4 normalized, alignment 8
	//   - 0x10 - short4, alignment 8
	//   - 0x11 - short4 normalized, alignment 4
	//   - 0x12 - short4, alignment 4
	//   - 0x13 - short4 normalized, alignment 4, reads the values with additional offset +2
	//   - 0x14 - short4, alignment 4, reads the values with additional offset +2
	//   - 0x15 - ubyte4 normalize, alignment 4
	//   - 0x16 - ubyte4, alignment 4
	//   - 0x17 - ubyte4 normalize, alignment 4, reads the values with additional offset +2
	//   - 0x18 - ubyte4, alignment 4, reads the values with additional offset +2
	//   - 0x19 - byte4 normalize, alignment 4
	//   - 0x1a - byte4, alignment 4
	//   - 0x1b - byte4 normalize, alignment 4, reads the values with additional offset +2
	//   - 0x1c - byte4, alignment 4, reads the values with additional offset +2

	uint offset = attribAccessInfo & 0x00ff;  // max offset is 255
	uint64_t addr = vertexDataPtr + offset;
	uint type = attribAccessInfo & 0xff00;

	// float4
	if(type == 0x0100)
		return AlignedVec4Ref(addr).value;

	// half4
	if(type == 0x0200) {
		uvec2 v = AlignedUVec2Ref(addr).value;
		return vec4(unpackHalf2x16(v[0]), unpackHalf2x16(v[1]));
	}
	if(type == 0x0300) {
		uvec2 v = UnalignedUVec2Ref(addr).value;
		return vec4(unpackHalf2x16(v[0]), unpackHalf2x16(v[1]));
	}
	if(type == 0x0400) {
		uvec3 v = UnalignedUVec3Ref(addr).value;
		v[0] = (v[0] >> 16) | (v[1] << 16);
		v[1] = (v[1] >> 16) | (v[2] << 16);
		return vec4(unpackHalf2x16(v[0]), unpackHalf2x16(v[1]));
	}

	// uint4
	if(type == 0x0500) {
		// alignment 16, normalize
		uvec4 v = AlignedUVec4Ref(addr).value;
		return vec4(float(v.x) / 0xffffffff, float(v.y) / 0xffffffff,
		            float(v.z) / 0xffffffff, float(v.w) / 0xffffffff);
	}
	if(type == 0x0600)
		// alignment 16, do not normalize
		return AlignedUVec4Ref(addr).value;

	// int4
	if(type == 0x0700) {
		// alignment 16, normalize
		ivec4 v = AlignedIVec4Ref(addr).value;
		return max(vec4(float(v.x) / 0x7fffffff, float(v.y) / 0x7fffffff,
		                float(v.z) / 0x7fffffff, float(v.w) / 0x7fffffff),
		           -1.);
	}
	if(type == 0x0800)
		// alignment 16, do not normalize
		return AlignedIVec4Ref(addr).value;

	// ushort4
	if(type == 0x0900) {
		// alignment 8, normalize
		uvec2 v = AlignedUVec2Ref(addr).value;
		return vec4(unpackUnorm2x16(v[0]), unpackUnorm2x16(v[1]));
	}
	if(type == 0x0a00) {
		// alignment 8, do not normalize
		uvec2 v = AlignedUVec2Ref(addr).value;
		return vec4(v[0] & 0xffff, v[0] >> 16, v[1] & 0xffff, v[1] >> 16);
	}
	if(type == 0x0b00) {
		// alignment 4, normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		return vec4(unpackUnorm2x16(v[0]), unpackUnorm2x16(v[1]));
	}
	if(type == 0x0c00) {
		// alignment 4, do not normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		return vec4(v[0] & 0xffff, v[0] >> 16, v[1] & 0xffff, v[1] >> 16);
	}
	if(type == 0x0d00) {
		// alignment 4, offset +2, normalize
		uvec3 v = UnalignedUVec3Ref(addr).value;
		v[0] = (v[0] >> 16) | (v[1] << 16);
		v[1] = (v[1] >> 16) | (v[2] << 16);
		return vec4(unpackUnorm2x16(v[0]), unpackUnorm2x16(v[1]));
	}
	if(type == 0x0e00) {
		// alignment 4, offset +2, do not normalize
		uvec3 v = UnalignedUVec3Ref(addr).value;
		v[0] = (v[0] >> 16) | (v[1] << 16);
		v[1] = (v[1] >> 16) | (v[2] << 16);
		return vec4(v[0] & 0xffff, v[0] >> 16, v[1] & 0xffff, v[1] >> 16);
	}

	// short4
	if(type == 0x0f00) {
		// alignment 8, normalize
		uvec2 v = AlignedUVec2Ref(addr).value;
		return vec4(unpackSnorm2x16(v[0]), unpackSnorm2x16(v[1]));
	}
	if(type == 0x1000) {
		// alignment 8, do not normalize
		uvec2 v = AlignedUVec2Ref(addr).value;
		ivec4 r = ivec4(int(v[0] & 0xffff), int(v[0] >> 16), int(v[1] & 0xffff), int(v[1] >> 16));
		r |= 0xffff0000 * (r >> 15);
		return vec4(r);
	}
	if(type == 0x1100) {
		// alignment 4, normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		return vec4(unpackSnorm2x16(v[0]), unpackSnorm2x16(v[1]));
	}
	if(type == 0x1200) {
		// alignment 4, do not normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		ivec4 r = ivec4(int(v[0] & 0xffff), int(v[0] >> 16), int(v[1] & 0xffff), int(v[1] >> 16));
		r |= 0xffff0000 * (r >> 15);
		return vec4(r);
	}
	if(type == 0x1300) {
		// alignment 4, offset +2, normalize
		uvec3 v = UnalignedUVec3Ref(addr).value;
		v[0] = (v[0] >> 16) | (v[1] << 16);
		v[1] = (v[1] >> 16) | (v[2] << 16);
		return vec4(unpackSnorm2x16(v[0]), unpackSnorm2x16(v[1]));
	}
	if(type == 0x1400) {
		// alignment 4, offset +2, do not normalize
		uvec3 v = UnalignedUVec3Ref(addr).value;
		v[0] = (v[0] >> 16) | (v[1] << 16);
		v[1] = (v[1] >> 16) | (v[2] << 16);
		ivec4 r = ivec4(int(v[0] & 0xffff), int(v[0] >> 16), int(v[1] & 0xffff), int(v[1] >> 16));
		r |= 0xffff0000 * (r >> 15);
		return vec4(r);
	}

	// ubyte
	if(type == 0x1500) {
		// alignment 4, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackUnorm4x8(v);
	}
	if(type == 0x1600) {
		// alignment 4, do not normalize
		uint v = AlignedUIntRef(addr).value;
		return vec4(v & 0xff, (v >> 8) & 0xff, (v >> 16) & 0xff, (v >> 24) & 0xff);
	}
	if(type == 0x1700) {
		// alignment 4, offset +2, normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		v[0] = (v[0] >> 16) | (v[1] << 16);
		return unpackUnorm4x8(v[0]);
	}
	if(type == 0x1800) {
		// alignment 4, offset +2, do not normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		v[0] = (v[0] >> 16) | (v[1] << 16);
		return vec4(v[0] & 0xff, (v[0] >> 8) & 0xff, (v[0] >> 16) & 0xff, (v[0] >> 24) & 0xff);
	}

	// byte
	if(type == 0x1900) {
		// alignment 4, normalize
		uint v = AlignedUIntRef(addr).value;
		return unpackSnorm4x8(v);
	}
	if(type == 0x1a00) {
		// alignment 4, do not normalize
		uint v = AlignedUIntRef(addr).value;
		ivec4 r = ivec4(int(v & 0xff), int((v >> 8) & 0xff), int((v >> 16) & 0xff), int((v >> 24) >> 0xff));
		r |= 0xffffff00 * (r >> 7);
		return vec4(r);
	}
	if(type == 0x1b00) {
		// alignment 4, offset +2, normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		v[0] = (v[0] >> 16) | (v[1] << 16);
		return unpackSnorm4x8(v[0]);
	}
	if(type == 0x1c00) {
		// alignment 4, offset +2, do not normalize
		uvec2 v = UnalignedUVec2Ref(addr).value;
		v[0] = (v[0] >> 16) | (v[1] << 16);
		ivec4 r = ivec4(int(v[0] & 0xff), int((v[0] >> 8) & 0xff), int((v[0] >> 16) & 0xff), int((v[0] >> 24) >> 0xff));
		r |= 0xffffff00 * (r >> 7);
		return vec4(r);
	}

	// try readVec3
	return vec4(readVec3(attribAccessInfo, vertexDataPtr), 1);
}
