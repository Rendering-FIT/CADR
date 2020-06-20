#pragma once

#include <vulkan/vulkan.hpp>

namespace CadR {

class Renderer;


class CADR_EXPORT Pipeline final {
protected:
	Renderer* _renderer;
	vk::Pipeline _pipeline;
	vk::PipelineLayout _pipelineLayout;
	std::vector<vk::DescriptorSetLayout> _descriptorSetLayouts;
public:

	Pipeline();
	Pipeline(Renderer* r);
	Pipeline(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>&& descriptorSetLayouts);
	Pipeline(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts);
	Pipeline(Renderer* r, vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>&& descriptorSetLayouts);
	Pipeline(Renderer* r, vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts);
	~Pipeline();

	void destroy();

	void init(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>&& descriptorSetLayouts);
	void init(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts);
	void init(Renderer* r, vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>&& descriptorSetLayouts);
	void init(Renderer* r, vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts);
	void set(vk::Pipeline pipeline);

	Renderer* renderer() const;
	vk::Pipeline get() const;
	vk::PipelineLayout layout() const;
	const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts() const;

};


}


// inline methods
#include <CadR/Renderer.h>
namespace CadR {

inline Pipeline::Pipeline() : _renderer(Renderer::get())  {}
inline Pipeline::Pipeline(Renderer* r) : _renderer(r)  {}
inline Pipeline::Pipeline(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>&& descriptorSetLayouts) : Pipeline(Renderer::get(), pipeline, pipelineLayout, descriptorSetLayouts)  {}
inline Pipeline::Pipeline(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts) : Pipeline(Renderer::get(), pipeline, pipelineLayout, descriptorSetLayouts)  {}
inline Pipeline::Pipeline(Renderer* r, vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>&& descriptorSetLayouts) : _renderer(r), _pipeline(pipeline), _pipelineLayout(pipelineLayout), _descriptorSetLayouts(descriptorSetLayouts)  {}
inline Pipeline::Pipeline(Renderer* r, vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts) : _renderer(r), _pipeline(pipeline), _pipelineLayout(pipelineLayout), _descriptorSetLayouts(descriptorSetLayouts)  {}
inline void Pipeline::init(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>&& descriptorSetLayouts)  { _pipeline=pipeline; _pipelineLayout=pipelineLayout; _descriptorSetLayouts=descriptorSetLayouts; }
inline void Pipeline::init(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts)  { _pipeline=pipeline; _pipelineLayout=pipelineLayout; _descriptorSetLayouts=descriptorSetLayouts; }
inline void Pipeline::init(Renderer* r, vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>&& descriptorSetLayouts)  { _renderer=r; init(pipeline, pipelineLayout, descriptorSetLayouts); }
inline void Pipeline::init(Renderer* r, vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts)  { _renderer=r; init(pipeline, pipelineLayout, descriptorSetLayouts); }
inline void Pipeline::set(vk::Pipeline pipeline)  { _pipeline=pipeline; }

inline Renderer* Pipeline::renderer() const  { return _renderer; }
inline vk::Pipeline Pipeline::get() const  { return _pipeline; }
inline vk::PipelineLayout Pipeline::layout() const  { return _pipelineLayout; }
inline const std::vector<vk::DescriptorSetLayout>& Pipeline::descriptorSetLayouts() const  { return _descriptorSetLayouts; }

}
