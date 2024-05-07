#pragma once

#include <array>
#include <vulkan/vulkan.hpp>

namespace CadR {
class VulkanDevice;
}


class PipelineLibrary {
protected:
	std::array<vk::ShaderModule, 8> _vertexShaders;
	std::array<vk::ShaderModule, 8> _fragmentShaders;
	std::array<vk::Pipeline, 32> _pipelines;
	CadR::VulkanDevice* _device = nullptr;
	vk::PipelineLayout _pipelineLayout;
public:

	PipelineLibrary() = default;
	~PipelineLibrary();

	void create(CadR::VulkanDevice& device, vk::Extent2D surfaceExtent, const vk::SpecializationInfo& si,
	            vk::RenderPass renderPass, vk::PipelineCache cache = nullptr);
	void create(CadR::VulkanDevice& device);
	void destroy() noexcept;

	static unsigned getShaderModuleIndex(bool phong, bool texturing, bool perVertexColor);
	vk::ShaderModule vertexShader(size_t index) const;
	vk::ShaderModule fragmentShader(size_t index) const;

	constexpr size_t numPipelines();
	static unsigned getPipelineIndex(bool phong, bool texturing, bool perVertexColor,
	                                 bool backFaceCulling, vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise);
	vk::Pipeline pipeline(size_t index) const;
	vk::Pipeline pipeline(bool phong, bool texturing, bool perVertexColor,
	                      bool backFaceCulling, vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise) const;

	vk::PipelineLayout pipelineLayout() const;

};


// inline functions
inline PipelineLibrary::~PipelineLibrary()  { destroy(); }
inline unsigned PipelineLibrary::getShaderModuleIndex(bool phong, bool texturing, bool perVertexColor)  { return (phong?4:0)+(texturing?2:0)+(perVertexColor?1:0); }
inline vk::ShaderModule PipelineLibrary::vertexShader(size_t index) const  { return _vertexShaders[index]; }
inline vk::ShaderModule PipelineLibrary::fragmentShader(size_t index) const  { return _fragmentShaders[index]; }
inline constexpr size_t PipelineLibrary::numPipelines()  { return _pipelines.size(); }
inline unsigned PipelineLibrary::getPipelineIndex(bool phong, bool texturing, bool perVertexColor, bool backFaceCulling, vk::FrontFace frontFace)  { return (frontFace==vk::FrontFace::eClockwise?0x10:0)+(backFaceCulling?8:0)+(phong?4:0)+(texturing?2:0)+(perVertexColor?1:0); }
inline vk::Pipeline PipelineLibrary::pipeline(size_t index) const  { return _pipelines[index]; }
inline vk::Pipeline PipelineLibrary::pipeline(bool phong, bool texturing, bool perVertexColor, bool backFaceCulling, vk::FrontFace frontFace) const  { return _pipelines[getPipelineIndex(phong, texturing, perVertexColor, backFaceCulling, frontFace)]; }
inline vk::PipelineLayout PipelineLibrary::pipelineLayout() const  { return _pipelineLayout; }
