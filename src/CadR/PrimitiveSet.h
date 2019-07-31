#pragma once

#include <cassert>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <CadR/Export.h>

namespace cadR {


/** PrimitiveSetGpuData are PrimitiveSet data that are stored
 *  in gpu buffers (usually Renderer::primitiveSetStorage())
 *  and processed by compute shader to produce
 *  indirect rendering buffer content.
 */
struct PrimitiveSetGpuData {
	unsigned count;         ///< Number of vertices of the primitive set.
	unsigned first;         ///< Index into the index buffer where the first index of the primitive set is stored.
	unsigned vertexOffset;  ///< Offset of the start of the allocated block of vertices or indices within AttribStorage. Thus, the real start index is first+vertexOffset. The value is computed and updated automatically.

	inline PrimitiveSetGpuData()  {}
	constexpr inline PrimitiveSetGpuData(unsigned count,unsigned first);
	constexpr inline PrimitiveSetGpuData(const PrimitiveSetGpuData&) = default;
};


/** DrawCommand represents single draw command, such as vkCmdDraw and vkCmdDrawIndexed.
 *
 *  DrawCommand together with DrawCommandGpuData are used for construction of
 *  indirect buffer contents or other means of high performance rendering.
 *
 *  DrawCommand carries information about indexing, topology and offset.
 *  Indexing signals indexed rendering (e.g. vkCmdDrawIndexed as opposed to vkCmdDraw),
 *  topology specifies type of rendered primitives (e.g. eTriangleList, eLineStrip,...)
 *  and offset4 points to the buffer of DrawCommandGpuData where the gpu specific
 *  draw command data are stored. Offset is given in multiple of 4 because shaders
 *  access memory as int array. Real memory offset is computed as offset4()*4.
 *
 *  Implementation note: The structure contains single 32-bit integer as we want to
 *  make sure it occupies only 4 bytes. (Bit fields are known not to be
 *  always tightly packed on MSVC.)
 */
struct CADR_EXPORT PrimitiveSet {
protected:

	unsigned data; // 32-bits, verified by assert in PrimitiveSet.cpp

public:

	constexpr inline vk::PrimitiveTopology topology() const;  ///< Returns topology of rendered geometry, e.g. eTriangleList, eLineStrip, etc.
	constexpr inline unsigned offset4() const;  ///< Returns offset into the buffer of DrawCommandGpuData where the gpu specific draw command data are stored. Offset is given in multiple of 4 because shaders access memory as int array.
	inline void setTopology(vk::PrimitiveTopology value);
	inline void setOffset4(unsigned value);
	inline void set(vk::PrimitiveTopology topology,unsigned offset4);  ///< Sets all DrawCommand state.

	inline PrimitiveSet()  {}  ///< Default constructor. Does nothing.
	constexpr inline PrimitiveSet(unsigned topology,unsigned offset4);  ///< Construct by parameters.

};


typedef std::vector<PrimitiveSet> PrimitiveSetList;



// inline methods
constexpr inline PrimitiveSetGpuData::PrimitiveSetGpuData(unsigned count_,unsigned first_)
	: count(count_), first(first_), vertexOffset(0)  {}
constexpr inline vk::PrimitiveTopology PrimitiveSet::topology() const  { return vk::PrimitiveTopology((data>>28)&0x0000000f); } // returns bits 28..31
constexpr inline unsigned PrimitiveSet::offset4() const  { return data&0x0fffffff; } // returns bits 0..27
inline void PrimitiveSet::setTopology(vk::PrimitiveTopology value)  { assert(unsigned(value)<=0xf); data=(data&0x0fffffff)|(unsigned(value)<<28); } // set bits 28..31
inline void PrimitiveSet::setOffset4(unsigned value)  { assert(value<=0x0fffffff); data=(data&0xf0000000)|value; } // set bits 0..27, value must fit to 28 bits
inline void PrimitiveSet::set(vk::PrimitiveTopology topology,unsigned offset4)  { assert(unsigned(topology)<=0xf && offset4<=0x0fffffff); data=(unsigned(topology)<<28)|offset4; } // set data, offset4 must fit to 27 bits
constexpr inline PrimitiveSet::PrimitiveSet(unsigned topology,unsigned offset4) : data((topology<<28)|offset4)  { assert(topology<=0xf && offset4<=0x0fffffff); }


}
