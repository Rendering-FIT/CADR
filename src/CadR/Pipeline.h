#ifndef CADR_PIPELINE_HEADER
# define CADR_PIPELINE_HEADER

# include <vulkan/vulkan.hpp>

namespace CadR {

class VulkanDevice;


class CADR_EXPORT Pipeline {
protected:
	vk::Pipeline _pipeline;
	vk::PipelineLayout _pipelineLayout;
	std::vector<vk::DescriptorSetLayout>* _descriptorSetLayoutList = nullptr;
public:

	inline Pipeline() = default;
	inline Pipeline(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>* descriptorSetLayoutList);

	inline void destroyPipeline(VulkanDevice& device);
	inline void destroyPipelineLayout(VulkanDevice& device);
	inline void destroyDescriptorSetLayouts(VulkanDevice& device);

	inline void init(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>* descriptorSetLayoutList);
	inline void set(vk::Pipeline pipeline);

	inline vk::Pipeline get() const;
	inline vk::PipelineLayout layout() const;
	inline const std::vector<vk::DescriptorSetLayout>& descriptorSetLayoutList() const;
	inline std::vector<vk::DescriptorSetLayout>& descriptorSetLayoutList();

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

inline Pipeline::Pipeline(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>* descriptorSetLayoutList)  : _pipeline(pipeline), _pipelineLayout(pipelineLayout), _descriptorSetLayoutList(descriptorSetLayoutList) {}
inline void Pipeline::destroyPipeline(VulkanDevice& device)  { device.destroy(_pipeline); }
inline void Pipeline::destroyPipelineLayout(VulkanDevice& device)  { device.destroy(_pipelineLayout); }
inline void Pipeline::destroyDescriptorSetLayouts(VulkanDevice& device)  { if(_descriptorSetLayoutList==nullptr) return; for(auto d : *_descriptorSetLayoutList) device.destroy(d); _descriptorSetLayoutList->clear(); }

inline void Pipeline::init(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, std::vector<vk::DescriptorSetLayout>* descriptorSetLayoutList)  { _pipeline=pipeline; _pipelineLayout=pipelineLayout; _descriptorSetLayoutList=descriptorSetLayoutList; }
inline void Pipeline::set(vk::Pipeline pipeline)  { _pipeline=pipeline; }

inline vk::Pipeline Pipeline::get() const  { return _pipeline; }
inline vk::PipelineLayout Pipeline::layout() const  { return _pipelineLayout; }
inline const std::vector<vk::DescriptorSetLayout>& Pipeline::descriptorSetLayoutList() const  { return *_descriptorSetLayoutList; }
inline std::vector<vk::DescriptorSetLayout>& Pipeline::descriptorSetLayoutList()  { return *_descriptorSetLayoutList; }

}
#endif
