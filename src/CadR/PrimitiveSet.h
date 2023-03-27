#pragma once

#include <cassert>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace CadR {


/** PrimitiveSetGpuData are stored in gpu buffers (usually Renderer::primitiveSetBuffer())
 *  and processed by compute shader to produce indirect rendering buffer content.
 */
struct PrimitiveSetGpuData {
	uint32_t count;         ///< Number of vertex indices used for the primitive set rendering.
	uint32_t first;         ///< Index into the index buffer where the first index of the primitive set is stored.
	uint32_t vertexOffset;  ///< Offset of the start of the allocated block of vertices within GeometryMemory. The value is computed and updated automatically by CadR.
	uint32_t userData;

	inline PrimitiveSetGpuData()  {}
	inline PrimitiveSetGpuData(uint32_t count, uint32_t first);
	inline PrimitiveSetGpuData(uint32_t count, uint32_t first, uint32_t userData);
	constexpr inline PrimitiveSetGpuData(uint32_t count, uint32_t first, uint32_t vertexOffset, uint32_t userData);
	constexpr inline PrimitiveSetGpuData(const PrimitiveSetGpuData&) = default;
};


/** PrimitiveSet represents the set of primitives that can be drawn by a single draw command,
 *  such as vkCmdDrawIndexed or vkCmdDrawIndexedIndirect.
 *
 *  PrimitiveSet together with PrimitiveSetGpuData are used for construction of
 *  indirect buffer contents or other means of high performance rendering.
 *
 *  PrimitiveSet carries information about topology and offset.
 *  Topology specifies type of rendered primitives (e.g. eTriangleList, eLineStrip,...)
 *  and offset4 points to the buffer of PrimitiveSetGpuData.
*   Offset is given in multiple of 4 because shaders access memory as int array.
 *  Real memory offset can be computed as offset4()*4.
 *
 *  Implementation note: The structure contains single 32-bit integer as we want to
 *  make sure it occupies only 4 bytes. (Bit fields are known not to be
 *  always tightly packed on MSVC.)
 */
struct CADR_EXPORT PrimitiveSet {  // 32-bits, verified by assert in PrimitiveSet.cpp
protected:

	uint32_t data;

public:

	constexpr inline vk::PrimitiveTopology topology() const;  ///< Returns topology of rendered geometry, e.g. eTriangleList, eLineStrip, etc.
	constexpr inline uint32_t offset4() const;  ///< Returns offset into the buffer of PrimitiveSetGpuData where the gpu specific draw command data are stored. Offset is given in multiple of 4 because shaders access memory as int array.
	inline void setTopology(vk::PrimitiveTopology value);
	inline void setOffset4(uint32_t value);
	inline void set(vk::PrimitiveTopology topology, uint32_t offset4);  ///< Sets whole PrimitiveSet state.

	inline PrimitiveSet()  {}  ///< Default constructor. Does nothing.
	constexpr inline PrimitiveSet(uint32_t topology, uint32_t offset4);  ///< Construct by parameters.
	constexpr inline PrimitiveSet(vk::PrimitiveTopology topology, uint32_t offset4);  ///< Construct by parameters.

};


typedef std::vector<PrimitiveSet> PrimitiveSetList;



// inline methods
inline PrimitiveSetGpuData::PrimitiveSetGpuData(uint32_t count_, uint32_t first_)
	: count(count_), first(first_)  {}
inline PrimitiveSetGpuData::PrimitiveSetGpuData(uint32_t count_, uint32_t first_, uint32_t userData_)
	: count(count_), first(first_), userData(userData_)  {}
constexpr inline PrimitiveSetGpuData::PrimitiveSetGpuData(uint32_t count_, uint32_t first_, uint32_t vertexOffset_, uint32_t userData_)
	: count(count_), first(first_), vertexOffset(vertexOffset_), userData(userData_)  {}
constexpr inline vk::PrimitiveTopology PrimitiveSet::topology() const  { return vk::PrimitiveTopology((data>>28)&0x0000000f); } // returns bits 28..31
constexpr inline uint32_t PrimitiveSet::offset4() const  { return data&0x0fffffff; } // returns bits 0..27
inline void PrimitiveSet::setTopology(vk::PrimitiveTopology value)  { assert(uint32_t(value)<=0xf); data=(data&0x0fffffff)|(uint32_t(value)<<28); } // set bits 28..31
inline void PrimitiveSet::setOffset4(uint32_t value)  { assert(value<=0x0fffffff); data=(data&0xf0000000)|value; } // set bits 0..27, value must fit to 28 bits
inline void PrimitiveSet::set(vk::PrimitiveTopology topology, uint32_t offset4)  { assert(uint32_t(topology)<=0xf && offset4<=0x0fffffff); data=(uint32_t(topology)<<28)|offset4; } // set data, offset4 must fit to 27 bits
constexpr inline PrimitiveSet::PrimitiveSet(uint32_t topology, uint32_t offset4) : data((topology<<28)|offset4)  { assert(topology<=0xf && offset4<=0x0fffffff); }
constexpr inline PrimitiveSet::PrimitiveSet(vk::PrimitiveTopology topology, uint32_t offset4) : PrimitiveSet(uint32_t(topology), offset4)  {}


}
