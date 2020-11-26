#pragma once

#include <vulkan/vulkan.hpp>

namespace CadR {

class Renderer;


class CADR_EXPORT Pipeline final {
protected:
	Renderer* _renderer;
	vk::Pipeline _pipeline;
	vk::PipelineLayout _pipelineLayout;
	std::vector<vk::DescriptorSetLayout>* _descriptorSetLayouts = nullptr;
public:

	Pipeline();
	Pipeline(Renderer* r);
	Pipeline(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>* descriptorSetLayouts);
	Pipeline(Renderer* r, vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>* descriptorSetLayouts);

	void destroyPipeline();
	void destroyPipelineLayout();
	void destroyDescriptorSetLayouts();

	void init(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>* descriptorSetLayouts);
	void init(Renderer* r, vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>* descriptorSetLayouts);
	void set(vk::Pipeline pipeline);

	Renderer* renderer() const;
	vk::Pipeline get() const;
	vk::PipelineLayout layout() const;
	std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts() const;

};


}


// inline methods
#include <CadR/Renderer.h>
#include <CadR/VulkanDevice.h>
namespace CadR {

inline Pipeline::Pipeline() : _renderer(Renderer::get())  {}
inline Pipeline::Pipeline(Renderer* r) : _renderer(r)  {}
inline Pipeline::Pipeline(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>* descriptorSetLayouts) : Pipeline(Renderer::get(), pipeline, pipelineLayout, descriptorSetLayouts)  {}
inline Pipeline::Pipeline(Renderer* r, vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>* descriptorSetLayouts) : _renderer(r), _pipeline(pipeline), _pipelineLayout(pipelineLayout), _descriptorSetLayouts(descriptorSetLayouts)  {}
inline void Pipeline::destroyPipeline()  { _renderer->device()->destroy(_pipeline); }
inline void Pipeline::destroyPipelineLayout()  { _renderer->device()->destroy(_pipelineLayout); }
inline void Pipeline::destroyDescriptorSetLayouts()  { if(_descriptorSetLayouts==nullptr) return; for(auto d : *_descriptorSetLayouts) _renderer->device()->destroy(d); _descriptorSetLayouts->clear(); }

inline void Pipeline::init(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>* descriptorSetLayouts)  { _pipeline=pipeline; _pipelineLayout=pipelineLayout; _descriptorSetLayouts=descriptorSetLayouts; }
inline void Pipeline::init(Renderer* r, vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>* descriptorSetLayouts)  { _renderer=r; init(pipeline, pipelineLayout, descriptorSetLayouts); }
inline void Pipeline::set(vk::Pipeline pipeline)  { _pipeline=pipeline; }

inline Renderer* Pipeline::renderer() const  { return _renderer; }
inline vk::Pipeline Pipeline::get() const  { return _pipeline; }
inline vk::PipelineLayout Pipeline::layout() const  { return _pipelineLayout; }
inline std::vector<vk::DescriptorSetLayout>& Pipeline::descriptorSetLayouts() const  { return *_descriptorSetLayouts; }

}
