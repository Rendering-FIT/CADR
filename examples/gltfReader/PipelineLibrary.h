#pragma once

#include <array>
#include <vulkan/vulkan.hpp>

namespace CadR {
class VulkanDevice;
}


class PipelineLibrary {
protected:
	static constexpr const size_t _numTrianglePipelines = 32;
	static constexpr const size_t _numLinePipelines     = 8;
	static constexpr const size_t _numPointPipelines    = 8;
	static constexpr const size_t _numPipelines = _numTrianglePipelines + _numLinePipelines + _numPointPipelines;
	std::array<vk::ShaderModule, 8> _vertexShaders;
	std::array<vk::ShaderModule, 8> _fragmentShaders;
	std::array<vk::Pipeline, _numPipelines> _pipelines;
	CadR::VulkanDevice* _device = nullptr;
	vk::PipelineLayout _pipelineLayout;
public:

	PipelineLibrary() = default;
	~PipelineLibrary();

	void create(CadR::VulkanDevice& device, vk::Extent2D surfaceExtent, const vk::SpecializationInfo& si,
	            vk::RenderPass renderPass, vk::PipelineCache cache = nullptr);
	void create(CadR::VulkanDevice& device);
	void destroy() noexcept;

	// shader modules
	static unsigned getShaderModuleIndex(bool phong, bool texturing, bool perVertexColor);
	vk::ShaderModule vertexShader(size_t index) const;
	vk::ShaderModule fragmentShader(size_t index) const;

	// all pipelines
	static constexpr size_t numPipelines();
	vk::Pipeline pipeline(size_t index) const;

	// triangle pipelines
	static constexpr size_t numTrianglePipelines();
	static constexpr unsigned getTrianglePipelineIndex(bool phong, bool texturing, bool perVertexColor,
		bool backFaceCulling, vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise);
	vk::Pipeline trianglePipeline(size_t index) const;
	vk::Pipeline trianglePipeline(bool phong, bool texturing, bool perVertexColor,
		bool backFaceCulling, vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise) const;

	// line pipelines
	static constexpr size_t numLinePipelines();
	static constexpr unsigned getLinePipelineIndex(bool phong, bool texturing, bool perVertexColor);
	vk::Pipeline linePipeline(size_t index) const;
	vk::Pipeline linePipeline(bool phong, bool texturing, bool perVertexColor) const;

	// point pipelines
	static constexpr size_t numPointPipelines();
	static constexpr unsigned getPointPipelineIndex(bool phong, bool texturing, bool perVertexColor);
	vk::Pipeline pointPipeline(size_t index) const;
	vk::Pipeline pointPipeline(bool phong, bool texturing, bool perVertexColor) const;

	vk::PipelineLayout pipelineLayout() const;

};


// inline functions
inline PipelineLibrary::~PipelineLibrary()  { destroy(); }
inline unsigned PipelineLibrary::getShaderModuleIndex(bool phong, bool texturing, bool perVertexColor)  { return (phong?4:0)+(texturing?2:0)+(perVertexColor?1:0); }
inline vk::ShaderModule PipelineLibrary::vertexShader(size_t index)   const  { return _vertexShaders[index]; }
inline vk::ShaderModule PipelineLibrary::fragmentShader(size_t index) const  { return _fragmentShaders[index]; }
inline constexpr size_t PipelineLibrary::numPipelines()          { return _numPipelines; }
inline constexpr size_t PipelineLibrary::numTrianglePipelines()  { return _numTrianglePipelines; }
inline constexpr size_t PipelineLibrary::numLinePipelines()      { return _numLinePipelines; }
inline constexpr size_t PipelineLibrary::numPointPipelines()     { return _numPointPipelines; }
inline constexpr unsigned PipelineLibrary::getTrianglePipelineIndex(bool phong, bool texturing, bool perVertexColor, bool backFaceCulling, vk::FrontFace frontFace)  { return (frontFace==vk::FrontFace::eClockwise?0x10:0)+(backFaceCulling?8:0)+(phong?4:0)+(texturing?2:0)+(perVertexColor?1:0); }
inline constexpr unsigned PipelineLibrary::getLinePipelineIndex(bool phong, bool texturing, bool perVertexColor)  { return _numTrianglePipelines+(phong?4:0)+(texturing?2:0)+(perVertexColor?1:0); }
inline constexpr unsigned PipelineLibrary::getPointPipelineIndex(bool phong, bool texturing, bool perVertexColor)  { return _numTrianglePipelines+_numLinePipelines+(phong?4:0)+(texturing?2:0)+(perVertexColor?1:0); }
inline vk::Pipeline PipelineLibrary::pipeline(size_t index) const  { return _pipelines[index]; }
inline vk::Pipeline PipelineLibrary::trianglePipeline(size_t index) const  { return _pipelines[index]; }
inline vk::Pipeline PipelineLibrary::linePipeline(size_t index)     const  { return _pipelines[_numTrianglePipelines+index]; }
inline vk::Pipeline PipelineLibrary::pointPipeline(size_t index)    const  { return _pipelines[_numTrianglePipelines+_numLinePipelines+index]; }
inline vk::Pipeline PipelineLibrary::trianglePipeline(bool phong, bool texturing, bool perVertexColor, bool backFaceCulling, vk::FrontFace frontFace) const  { return _pipelines[getTrianglePipelineIndex(phong, texturing, perVertexColor, backFaceCulling, frontFace)]; }
inline vk::Pipeline PipelineLibrary::linePipeline(bool phong, bool texturing, bool perVertexColor) const  { return _pipelines[getLinePipelineIndex(phong, texturing, perVertexColor)]; }
inline vk::Pipeline PipelineLibrary::pointPipeline(bool phong, bool texturing, bool perVertexColor) const  { return _pipelines[getPointPipelineIndex(phong, texturing, perVertexColor)]; }
inline vk::PipelineLayout PipelineLibrary::pipelineLayout() const  { return _pipelineLayout; }
