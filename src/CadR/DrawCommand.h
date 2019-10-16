#pragma once

#include <CadR/Export.h>

namespace CadR {


/** DrawCommandGpuData is data structure associated with DrawCommand and it is stored in GPU buffers,
 *  usually in RenderingContext::drawCommandBuffer().
 *  Allocation management of DrawCommandGpuData buffer is usually provided
 *  by DrawCommandAllocationManager.
 *
 *  \sa RenderingContext::objectBuffer()
 */
struct DrawCommandGpuData final {
	uint32_t primitiveSetOffset4;
	uint32_t matrixListControlOffset4;
	uint32_t stateSetOffset4;
	uint32_t userData;  ///< Provides 16-byte structure alignment and opportunuty to store user data.

	DrawCommandGpuData()  {}
	DrawCommandGpuData(uint32_t primitiveOffset4,uint32_t matrixListControlOffset4,uint32_t stateSetOffset4);
	constexpr DrawCommandGpuData(uint32_t primitiveOffset4,uint32_t matrixListControlOffset4,
	                             uint32_t stateSetOffset4,uint32_t userData);
};


/** DrawCommand structure holds the information of a single
 *  draw command managed by ge::rg::Object.
 *  It holds the index into the buffer of DrawCommandGpuData.
 */
class CADR_EXPORT DrawCommand final {
protected:
	uint32_t _index;
public:
	constexpr uint32_t index() const;
	void setIndex(uint32_t value);

	DrawCommand() {}
	constexpr DrawCommand(uint32_t index);
};


// inline and template methods
inline DrawCommandGpuData::DrawCommandGpuData(uint32_t primitiveSetOffset4_,uint32_t matrixListControlOffset4_,uint32_t stateSetOffset4_)
	: primitiveSetOffset4(primitiveSetOffset4_), matrixListControlOffset4(matrixListControlOffset4_), stateSetOffset4(stateSetOffset4_)  {}
inline constexpr DrawCommandGpuData::DrawCommandGpuData(uint32_t primitiveSetOffset4_,uint32_t matrixListControlOffset4_,uint32_t stateSetOffset4_,uint32_t userData_)
	: primitiveSetOffset4(primitiveSetOffset4_), matrixListControlOffset4(matrixListControlOffset4_), stateSetOffset4(stateSetOffset4_), userData(userData_)  {}
inline constexpr uint32_t DrawCommand::index() const  { return _index; }
inline void DrawCommand::setIndex(uint32_t value)  { _index=value; }
inline constexpr DrawCommand::DrawCommand(uint32_t index) : _index(index)  {}


}
