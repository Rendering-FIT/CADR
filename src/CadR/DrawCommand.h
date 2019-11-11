#pragma once

#include <CadR/AllocationManagers.h>
#include <CadR/Export.h>
#include <cstdint>

namespace CadR {

class Renderer;


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
class CADR_EXPORT DrawCommand : public ItemAllocation {
public:
	using ItemAllocation::alloc;
	using ItemAllocation::free;
	uint32_t alloc(Renderer* r);
	void free(Renderer* r);

	using ItemAllocation::ItemAllocation;
	DrawCommand(Renderer* r);

	DrawCommand(DrawCommand&&) = delete;
	DrawCommand(const DrawCommand&) = delete;
	DrawCommand& operator=(DrawCommand&&) = delete;
	DrawCommand& operator=(const DrawCommand&) = delete;
	DrawCommand(DrawCommand&& dc,Renderer* r);
	using ItemAllocation::assign;
	void assign(DrawCommand&& rhs,Renderer* r);
	inline void* operator new(size_t,CadR::DrawCommand& p) noexcept { return &p; }
	inline void operator delete(void*,CadR::DrawCommand&) noexcept {}
};


}

// inline and template methods
#include <CadR/Renderer.h>
namespace CadR {

inline DrawCommandGpuData::DrawCommandGpuData(uint32_t primitiveSetOffset4_,uint32_t matrixListControlOffset4_,uint32_t stateSetOffset4_)
	: primitiveSetOffset4(primitiveSetOffset4_), matrixListControlOffset4(matrixListControlOffset4_), stateSetOffset4(stateSetOffset4_)  {}
inline constexpr DrawCommandGpuData::DrawCommandGpuData(uint32_t primitiveSetOffset4_,uint32_t matrixListControlOffset4_,uint32_t stateSetOffset4_,uint32_t userData_)
	: primitiveSetOffset4(primitiveSetOffset4_), matrixListControlOffset4(matrixListControlOffset4_), stateSetOffset4(stateSetOffset4_), userData(userData_)  {}
inline uint32_t DrawCommand::alloc(Renderer* r)  { return ItemAllocation::alloc(r->drawCommandAllocationManager()); }
inline void DrawCommand::free(Renderer* r)  { ItemAllocation::free(r->drawCommandAllocationManager()); }
inline DrawCommand::DrawCommand(Renderer* r) : ItemAllocation(r->drawCommandAllocationManager())  {}
inline DrawCommand::DrawCommand(DrawCommand&& dc,Renderer* r) : ItemAllocation(std::move(dc),r->drawCommandAllocationManager())  {}
inline void DrawCommand::assign(DrawCommand&& rhs,Renderer* r)  { ItemAllocation::assign(std::move(rhs),r->drawCommandAllocationManager()); }

}
