#pragma once

#include <cassert>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <CADR/Export.h>

namespace cd {


/** DrawCommandGpuData are DrawCommand data that are stored
 *  in gpu buffers (usually Renderer::drawCommandStorage())
 *  and processed by compute shader to produce
 *  indirect rendering buffer content.
 */
struct DrawCommandGpuData {
	unsigned countAndIndexedFlag; ///< Number of vertices of the primitive and indexing flag on the highest bit indicating whether glDrawArrays or glDrawElements should be used for rendering.
	unsigned first;               ///< Index of the first vertex or first index of the primitive.
	unsigned vertexOffset;        ///< Offset of the start of the allocated block of vertices or indices within AttribStorage. Thus, the real start index is first+vertexOffset. The value is computed and updated automatically.

	inline DrawCommandGpuData()  {}
	constexpr inline DrawCommandGpuData(unsigned countAndIndexedFlag,unsigned first);
	constexpr inline DrawCommandGpuData(unsigned count,unsigned first,bool indexed);
	constexpr inline DrawCommandGpuData(const DrawCommandGpuData&) = default;

	constexpr inline unsigned count() const  { return countAndIndexedFlag&0x7fffffff; }
	constexpr inline bool indexed() const  { return (countAndIndexedFlag&0x80000000)!=0; }
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
struct CADR_EXPORT DrawCommand {
protected:

	unsigned data; // 32-bits, verified by assert in DrawCommand.cpp

public:

	constexpr inline bool indexed() const;  ///< Returns true if the command uses index rendering, e.g. vkCmdDrawIndexed.
	constexpr inline vk::PrimitiveTopology topology() const;  ///< Returns topology of rendered geometry, e.g. eTriangleList, eLineStrip, etc.
	constexpr inline unsigned offset4() const;  ///< Returns offset into the buffer of DrawCommandGpuData where the gpu specific draw command data are stored. Offset is given in multiple of 4 because shaders access memory as int array.
	inline void setIndexed(bool value);
	inline void setTopology(vk::PrimitiveTopology value);
	inline void setOffset4(unsigned value);
	inline void set(bool indexed,vk::PrimitiveTopology topology,unsigned offset4);  ///< Sets all DrawCommand state.

	inline DrawCommand()  {}  ///< Default constructor. Does nothing.
	constexpr inline DrawCommand(bool indexed,unsigned topology,unsigned offset4);  ///< Construct by parameters.

};


typedef std::vector<DrawCommand> DrawCommandList;



// inline methods
constexpr inline DrawCommandGpuData::DrawCommandGpuData(unsigned countAndIndexedFlag_,unsigned first_)
	: countAndIndexedFlag(countAndIndexedFlag_), first(first_), vertexOffset(0)  {}
constexpr inline DrawCommandGpuData::DrawCommandGpuData(unsigned count,unsigned first_,bool indexed)
	: countAndIndexedFlag(count|(indexed?0x80000000:0)), first(first_), vertexOffset(0)  {}
constexpr inline bool DrawCommand::indexed() const  { return (data>>31)!=0; } // returns bit 31
constexpr inline vk::PrimitiveTopology DrawCommand::topology() const  { return vk::PrimitiveTopology((data>>27)&0x0000000f); } // returns bits 27..30
constexpr inline unsigned DrawCommand::offset4() const  { return data&0x07ffffff; } // returns bits 0..26
inline void DrawCommand::setIndexed(bool value)  { data=(data&0x7fffffff)|(value<<31); } // set bit 31
inline void DrawCommand::setTopology(vk::PrimitiveTopology value)  { assert(unsigned(value)<=0xf); data=(data&0x87ffffff)|(unsigned(value)<<27); } // set bits 27..30
inline void DrawCommand::setOffset4(unsigned value)  { assert(value<=0x07ffffff); data=(data&0xf8000000)|value; } // set bits 0..26, value must fit to 27 bits
inline void DrawCommand::set(bool indexed,vk::PrimitiveTopology topology,unsigned offset4)  { assert(unsigned(topology)<=0xf && offset4<=0x07ffffff); data=(indexed<<31)|(unsigned(topology)<<27)|offset4; } // set data, offset4 must fit to 27 bits
#if defined(_MSC_VER) && _MSC_VER<=1900 // no constexpr assert support in MSVC 2015
constexpr inline DrawCommand::DrawCommand(bool indexed,unsigned topology,unsigned offset4) : data((indexed<<31)|(topology<<27)|offset4)  {}
#else
constexpr inline DrawCommand::DrawCommand(bool indexed,unsigned topology,unsigned offset4) : data((indexed<<31)|(topology<<27)|offset4)  { assert(topology<=0xf && offset4<=0x07ffffff); }
#endif


}
