#pragma once

namespace CadR {


/** Frame info contains information about the rendered frame.
 *
 *  The timestamps in the structure can be converted to time by information provided by Renderer,
 *  particularly Renderer::cpuTimestampPeriod() and Renderer::gpuTimestampPeriod().
 */
struct FrameInfo
{
	std::size_t frameNumber;  ///< Frame number uniquely identifies particular rendered frame. It monotonically increases with each frame scheduled for rendering. The first frame number is 0.
	uint64_t beginFrameGpu;  ///< Gpu timestamp taken at the beginning of the frame. Gpu timestamps are based on gpu time and gpuTimestampPeriod. Values of beginFrameCpu and beginFrameGpu are retrieved at about the same time, so their different values might be used to relate cpu and gpu timestamps.
	uint64_t beginFrameCpu;  ///< Cpu timestamp taken at the beginning of the frame. Cpu timestamps are based on cpu time and cpuTimestampPeriod. Values of beginFrameCpu and beginFrameGpu are retrieved at about the same time, so their different values might be used to relate cpu and gpu timestamps.
	uint64_t endFrameCpu;  ///< Cpu timestamp of the end of frame. The timestamp marks the moment when all the work was submitted to gpu. Although gpu still work for a while, cpu is now available for other tasks.
	uint64_t beginRenderingGpu;  ///< Gpu timestamp of the beginning of the frame rendering.
	uint64_t endRenderingGpu;  ///< Gpu timestamp of the end of frame rendering.

	static inline constexpr uint32_t gpuTimestampPoolSize = 2;  ///< Number of gpu timestamps per frame.
};


}
