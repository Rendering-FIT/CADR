#ifndef CADR_PIPELINE_HEADER
# define CADR_PIPELINE_HEADER

# include <vulkan/vulkan.hpp>

namespace CadR {

class VulkanDevice;


class CADR_EXPORT Pipeline {
protected:
	vk::Pipeline _pipeline;
	vk::PipelineLayout _pipelineLayout;
	const std::vector<vk::DescriptorSetLayout>* _descriptorSetLayoutList = nullptr;
public:

	// construction and destruction
	inline Pipeline() = default;
	inline Pipeline(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>* descriptorSetLayoutList) noexcept;
	inline void destroyPipeline(VulkanDevice& device) noexcept;
	inline void destroyPipelineLayout(VulkanDevice& device) noexcept;
	inline void destroyDescriptorSetLayouts(VulkanDevice& device) noexcept;

	// set functions
	inline void init(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, const std::vector<vk::DescriptorSetLayout>* descriptorSetLayoutList) noexcept;
	inline void set(vk::Pipeline pipeline) noexcept;

	// getters
	inline vk::Pipeline get() const;
	inline vk::PipelineLayout layout() const;
	inline const std::vector<vk::DescriptorSetLayout>& descriptorSetLayoutList() const;

};


}

#endif


// inline methods
#if !defined(CADR_PIPELINE_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_PIPELINE_INLINE_FUNCTIONS
# define CADR_NO_INLINE_FUNCTIONS
# include <CadR/VulkanDevice.h>
# undef CADR_NO_INLINE_FUNCTIONS
namespace CadR {

inline Pipeline::Pipeline(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>* descriptorSetLayoutList) noexcept  : _pipeline(pipeline), _pipelineLayout(pipelineLayout), _descriptorSetLayoutList(descriptorSetLayoutList) {}
inline void Pipeline::destroyPipeline(VulkanDevice& device) noexcept  { device.destroy(_pipeline); }
inline void Pipeline::destroyPipelineLayout(VulkanDevice& device) noexcept  { device.destroy(_pipelineLayout); }
inline void Pipeline::destroyDescriptorSetLayouts(VulkanDevice& device) noexcept  { if(_descriptorSetLayoutList==nullptr) return; for(auto d : *_descriptorSetLayoutList) device.destroy(d); }

inline void Pipeline::init(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, const std::vector<vk::DescriptorSetLayout>* descriptorSetLayoutList) noexcept  { _pipeline=pipeline; _pipelineLayout=pipelineLayout; _descriptorSetLayoutList=descriptorSetLayoutList; }
inline void Pipeline::set(vk::Pipeline pipeline) noexcept  { _pipeline=pipeline; }

inline vk::Pipeline Pipeline::get() const  { return _pipeline; }
inline vk::PipelineLayout Pipeline::layout() const  { return _pipelineLayout; }
inline const std::vector<vk::DescriptorSetLayout>& Pipeline::descriptorSetLayoutList() const  { return *_descriptorSetLayoutList; }

}
#endif
