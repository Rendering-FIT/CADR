#pragma once

#include <array>
#include <list>
#include <map>
#include <tuple>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <glm/mat4x4.hpp>
#include <CadPL/ShaderLibrary.h>
#include <CadR/Pipeline.h>

namespace CadR {
class VulkanDevice;
}

namespace CadPL {

// forward declarations
class SharedPipeline;
class PipelineFamily;
class PipelineLibrary;


struct PipelineState {

	enum class ViewportAndScissorHandling {
		Value,  //< viewport and scissor is specified by the value of PipelineState::viewport and PipelineState::scissor
		DynamicState,  //< viewport and scissor is specified by Vulkan dynamic state; values of PipelineState::viewport and PipelineState::scissor are ignored
		SetFunction,  //< PipelineState::viewport and PipelineState::scissor values are set each time PipelineLibrary::setProjectionViewportAndScissor() is called
	};
	ViewportAndScissorHandling viewportAndScissorHandling = ViewportAndScissorHandling::SetFunction;
	unsigned projectionIndex = 0;  //< When ShaderState::projectionHandling is set to ProjectionHandling::PerspectivePushAndSpecializationConstants, it is the index into projection matrix list passed as parameter into PipelineLibrary::setProjectionViewportAndScissor() function.
	unsigned viewportIndex = 0;  //< When PipelineState::viewportAndScissorHandling is set to ViewportAndScissorHandling::SetFunction, it is the index into viewport list passed as parameter into PipelineLibrary::setProjectionViewportAndScissor() function.
	unsigned scissorIndex = 0;  //< When PipelineState::viewportAndScissorHandling is set to ViewportAndScissorHandling::SetFunction, it is the index into scissor list passed as parameter into PipelineLibrary::setProjectionViewportAndScissor() function.
	vk::Viewport viewport;  //< Viewport set by the user, or by PipelineLibrary::setProjectionViewportAndScissor() if PipelineState::viewportAndScissorHandling is set to ViewportAndScissorHandling::SetFunction.
	vk::Rect2D scissor;  //< Scissor set by the user, or by PipelineLibrary::setProjectionViewportAndScissor() if PipelineState::viewportAndScissorHandling is set to ViewportAndScissorHandling::SetFunction.

	vk::CullModeFlagBits cullMode = vk::CullModeFlagBits::eBack;
	vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
	bool depthBiasDynamicState = false;
	bool depthBiasEnable = false;
	float depthBiasConstantFactor;
	float depthBiasClamp;
	float depthBiasSlopeFactor;
	bool lineWidthDynamicState = false;
	float lineWidth = 1.f;
	vk::SampleCountFlagBits rasterizationSamples = vk::SampleCountFlagBits::e1;
	bool sampleShadingEnable = false;
	float minSampleShading;
	bool depthTestEnable = true;
	bool depthWriteEnable = true;

	struct BlendAttachmentState {
		bool blendEnable = false;
		vk::BlendFactor srcColorBlendFactor = vk::BlendFactor::eZero;
		vk::BlendFactor dstColorBlendFactor = vk::BlendFactor::eZero;
		vk::BlendOp colorBlendOp = vk::BlendOp::eAdd;
		vk::BlendFactor srcAlphaBlendFactor = vk::BlendFactor::eZero;
		vk::BlendFactor dstAlphaBlendFactor = vk::BlendFactor::eZero;
		vk::BlendOp alphaBlendOp = vk::BlendOp::eAdd;
		vk::ColorComponentFlags colorWriteMask =
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

		bool operator<(const BlendAttachmentState& rhs) const;
	};
	std::vector<BlendAttachmentState> blendState;

	vk::RenderPass renderPass = nullptr;
	uint32_t subpass = 0;

	bool operator<(const PipelineState& rhs) const;

};


class CADPL_EXPORT SharedPipeline {
protected:
	void* _pipelineObject = nullptr;
public:

	// construction and destruction
	SharedPipeline() = default;
	~SharedPipeline() noexcept;

	// copy and move constructors and operators
	SharedPipeline(SharedPipeline&& other) noexcept;
	SharedPipeline(const SharedPipeline& other) noexcept;
	SharedPipeline& operator=(SharedPipeline&& rhs) noexcept;
	SharedPipeline& operator=(const SharedPipeline& rhs) noexcept;

	// functions
	void reset() noexcept;

	// getters
	const CadR::Pipeline* cadrPipeline() const;
	const PipelineFamily* pipelineFamily() const;
	const PipelineState* pipelineState() const;

protected:
	friend PipelineFamily;
	friend PipelineLibrary;
	SharedPipeline(void* pipelineOwner) noexcept;
	void replacePipelineHandle(vk::Pipeline pipeline, CadR::VulkanDevice& device) noexcept;
};


class CADPL_EXPORT PipelineFamily {
protected:

	CadR::VulkanDevice* _device;
	PipelineLibrary* _pipelineLibrary;
	std::map<ShaderState, PipelineFamily>::iterator _mapIterator;

	SharedShaderModule _vertexShader;
	SharedShaderModule _geometryShader;
	SharedShaderModule _fragmentShader;
	vk::PrimitiveTopology _primitiveTopology;

	struct PipelineObject {
		size_t referenceCounter;
		CadR::Pipeline cadrPipeline;
		PipelineFamily* pipelineFamily;
		std::map<PipelineState, PipelineObject>::iterator mapIterator;
	};

	std::map<PipelineState, PipelineObject> _pipelineMap;

	static void refPipeline(void* pipelineObject) noexcept;
	static void unrefPipeline(void* pipelineObject) noexcept;
	static void destroyPipeline(void* pipelineObject) noexcept;

	friend SharedPipeline;
	friend PipelineLibrary;

public:

	PipelineFamily() = delete;
	PipelineFamily(PipelineLibrary& pipelineLibrary) noexcept;
	~PipelineFamily() noexcept;

	SharedPipeline getOrCreatePipeline(const PipelineState& pipelineState);
	SharedPipeline getPipeline(const PipelineState& pipelineState);
	const std::map<PipelineState, PipelineObject>& pipelineMap() const;

	const ShaderState& shaderState() const;

};


class CADPL_EXPORT PipelineLibrary {
protected:

	ShaderLibrary* _shaderLibrary;
	std::map<ShaderState, PipelineFamily> _pipelineFamilyMap;
	CadR::VulkanDevice* _device;
	vk::PipelineCache _pipelineCache;

	std::vector<std::array<float,6>> _specializationData;
	std::vector<vk::Viewport> _viewportList;
	std::vector<vk::Rect2D> _scissorList;

	struct CreationDataSet;  // forward declaration

	struct CreationDataBatch
	{
		static const size_t numPipelines = 16;
		static const size_t numAttachmentsPerPipeline = 3;
		CreationDataSet* creationDataSet;
		std::array<SharedPipeline,numPipelines> sharedPipelineList;  //< Pipelines that are going to be created.
		unsigned numSharedPipelines = 0;
		std::array<vk::PipelineShaderStageCreateInfo,numPipelines*3> shaderStageList;
		unsigned numShaderStages = 0;
		std::array<vk::PipelineInputAssemblyStateCreateInfo,numPipelines> inputAssemblyStateList;
		unsigned numInputAssemblyStates = 0;
		std::array<std::tuple<vk::PipelineViewportStateCreateInfo,vk::Viewport,vk::Rect2D>,numPipelines> viewportStateList;
		unsigned numViewportStates = 0;
		std::array<vk::PipelineRasterizationStateCreateInfo,numPipelines> rasterizationStateList;
		unsigned numRasterizationStates = 0;
		std::array<vk::PipelineMultisampleStateCreateInfo,numPipelines> multisampleStateList;
		unsigned numMultisampleStates = 0;
		std::array<vk::PipelineDepthStencilStateCreateInfo,numPipelines> depthStencilStateList;
		unsigned numDepthStencilStates = 0;
		std::array<vk::PipelineColorBlendAttachmentState,numPipelines> colorBlendAttachmentStateList;
		unsigned numColorBlendAttachmentStates = 0;
		std::array<vk::PipelineColorBlendStateCreateInfo,numPipelines> colorBlendStateList;
		unsigned numColorBlendStates = 0;
		std::array<vk::GraphicsPipelineCreateInfo,numPipelines> createInfoList;
		unsigned numCreateInfos = 0;
		CreationDataBatch(CreationDataSet* creationDataSet);
		bool isFull() const;
		void append(SharedPipeline&& sharedPipeline, const PipelineState& pipelineState);
		[[nodiscard]] std::array<vk::Pipeline,PipelineLibrary::CreationDataBatch::numPipelines>
			createPipelines(CadR::VulkanDevice& device, vk::PipelineCache pipelineCache);
	};
	struct CreationDataSet
	{
		std::vector<std::tuple<std::array<float,6>, vk::SpecializationInfo>> specializationList;
		std::vector<vk::Viewport> viewportList;
		std::vector<vk::Rect2D> scissorList;

		std::list<CreationDataBatch> batchList;

		CreationDataSet(const PipelineLibrary& pipelineLibrary);
		void append(SharedPipeline&& sharedPipeline, const PipelineState& pipelineState);
		void createPipelines(const PipelineLibrary& pipelineLibrary);
	};

	friend PipelineFamily;

public:

	// construction and destruction
	PipelineLibrary() noexcept;
	PipelineLibrary(ShaderLibrary& shaderLibrary, vk::PipelineCache pipelineCache = nullptr);
	~PipelineLibrary() noexcept;
	void init(ShaderLibrary& shaderLibrary, vk::PipelineCache pipelineCache = nullptr);

	// projection, viewport and scissor for pipelines
	void setProjectionViewportAndScissor(const glm::mat4x4& projectionMatrix, const vk::Viewport& viewport, const vk::Rect2D& scissor);
	void setProjectionViewportAndScissor(const std::vector<glm::mat4x4>& projectionMatrixList,
		const std::vector<vk::Viewport>& viewportList, const std::vector<vk::Rect2D>& scissorList);

	// synchronous API to get and create pipelines
	SharedPipeline getOrCreatePipeline(const ShaderState& shaderState, const PipelineState& pipelineState);
	SharedPipeline getPipeline(const ShaderState& shaderState, const PipelineState& pipelineState);

	// getters
	CadR::VulkanDevice& device() const;
	ShaderLibrary& shaderLibrary() const;
	vk::PipelineCache pipelineCache() const;
	vk::PipelineLayout pipelineLayout() const;
	vk::DescriptorSetLayout descriptorSetLayout() const;
	const std::vector<vk::DescriptorSetLayout>& descriptorSetLayoutList() const;

};


// inline functions
inline SharedPipeline::SharedPipeline(void* pipelineObject) noexcept  : _pipelineObject(pipelineObject) { PipelineFamily::refPipeline(pipelineObject); }
inline SharedPipeline::~SharedPipeline() noexcept  { if(_pipelineObject) PipelineFamily::unrefPipeline(_pipelineObject); }
inline SharedPipeline::SharedPipeline(SharedPipeline&& other) noexcept  : _pipelineObject(other._pipelineObject) { other._pipelineObject=nullptr; }
inline SharedPipeline::SharedPipeline(const SharedPipeline& other) noexcept  : _pipelineObject(other._pipelineObject) { if(_pipelineObject) PipelineFamily::refPipeline(_pipelineObject); }
inline SharedPipeline& SharedPipeline::operator=(SharedPipeline&& rhs) noexcept  { if(_pipelineObject) PipelineFamily::unrefPipeline(_pipelineObject); _pipelineObject=rhs._pipelineObject; rhs._pipelineObject=nullptr; return *this; }
inline SharedPipeline& SharedPipeline::operator=(const SharedPipeline& rhs) noexcept  { if(_pipelineObject) PipelineFamily::unrefPipeline(_pipelineObject); _pipelineObject=rhs._pipelineObject; if(_pipelineObject) PipelineFamily::refPipeline(_pipelineObject); return *this; }
inline void SharedPipeline::reset() noexcept  { if(!_pipelineObject) return; PipelineFamily::unrefPipeline(_pipelineObject); _pipelineObject=nullptr; }
inline const CadR::Pipeline* SharedPipeline::cadrPipeline() const  { return (_pipelineObject) ? &static_cast<PipelineFamily::PipelineObject*>(_pipelineObject)->cadrPipeline : nullptr; }
inline const PipelineFamily* SharedPipeline::pipelineFamily() const  { return (_pipelineObject) ? static_cast<PipelineFamily::PipelineObject*>(_pipelineObject)->pipelineFamily : nullptr; }
inline const PipelineState* SharedPipeline::pipelineState() const  { return (_pipelineObject) ? &static_cast<PipelineFamily::PipelineObject*>(_pipelineObject)->mapIterator->first : nullptr; }
inline void SharedPipeline::replacePipelineHandle(vk::Pipeline newPipeline, CadR::VulkanDevice& device) noexcept  { assert(_pipelineObject && "SharedPipeline was not properly initialized yet."); CadR::Pipeline& cadrPipeline = static_cast<PipelineFamily::PipelineObject*>(_pipelineObject)->cadrPipeline; device.destroy(cadrPipeline.get()); cadrPipeline.set(newPipeline); }

inline void PipelineFamily::refPipeline(void* pipelineObject) noexcept  { PipelineObject* po=static_cast<PipelineObject*>(pipelineObject); po->referenceCounter++; }
inline void PipelineFamily::unrefPipeline(void* pipelineObject) noexcept  { PipelineObject* po=static_cast<PipelineObject*>(pipelineObject); if(po->referenceCounter==1) PipelineFamily::destroyPipeline(po); else po->referenceCounter--; }
inline PipelineFamily::PipelineFamily(PipelineLibrary& pipelineLibrary) noexcept  : _device(pipelineLibrary._device), _pipelineLibrary(&pipelineLibrary) {}
inline SharedPipeline PipelineFamily::getPipeline(const PipelineState& pipelineState)  { auto it=_pipelineMap.find(pipelineState); return (it!=_pipelineMap.end()) ? SharedPipeline(&it->second) : SharedPipeline(); }
inline const std::map<PipelineState, PipelineFamily::PipelineObject>& PipelineFamily::pipelineMap() const  { return _pipelineMap; }
inline const ShaderState& PipelineFamily::shaderState() const  { return _mapIterator->first; }

inline PipelineLibrary::CreationDataBatch::CreationDataBatch(PipelineLibrary::CreationDataSet* creationDataSet_)  : creationDataSet(creationDataSet_) {}
inline bool PipelineLibrary::CreationDataBatch::isFull() const  { return numSharedPipelines == numPipelines; }
inline void PipelineLibrary::CreationDataSet::append(SharedPipeline&& sharedPipeline, const PipelineState& pipelineState)  { if(batchList.empty() || batchList.back().isFull()) batchList.emplace_back(this); batchList.back().append(std::move(sharedPipeline), pipelineState); }

inline PipelineLibrary::PipelineLibrary() noexcept  : _shaderLibrary(nullptr), _device(nullptr) {}
inline PipelineLibrary::PipelineLibrary(ShaderLibrary& shaderLibrary, vk::PipelineCache pipelineCache)  : _shaderLibrary(&shaderLibrary), _device(&shaderLibrary.device()), _pipelineCache(pipelineCache) {}
inline void PipelineLibrary::init(ShaderLibrary& shaderLibrary, vk::PipelineCache pipelineCache)  { _device=&shaderLibrary.device(); _shaderLibrary=&shaderLibrary; _pipelineCache=pipelineCache; }
inline void PipelineLibrary::setProjectionViewportAndScissor(const glm::mat4x4& projectionMatrix, const vk::Viewport& viewport, const vk::Rect2D& scissor)  { setProjectionViewportAndScissor(std::vector{projectionMatrix}, std::vector{viewport}, std::vector{scissor}); }
inline SharedPipeline PipelineLibrary::getPipeline(const ShaderState& shaderState, const PipelineState& pipelineState)  { auto it=_pipelineFamilyMap.find(shaderState); return (it!=_pipelineFamilyMap.end()) ? it->second.getPipeline(pipelineState) : SharedPipeline(); }
inline CadR::VulkanDevice& PipelineLibrary::device() const  { return *_device; }
inline ShaderLibrary& PipelineLibrary::shaderLibrary() const  { return *_shaderLibrary; }
inline vk::PipelineCache PipelineLibrary::pipelineCache() const  { return _pipelineCache; }
inline vk::PipelineLayout PipelineLibrary::pipelineLayout() const  { return _shaderLibrary->pipelineLayout(); }
inline vk::DescriptorSetLayout PipelineLibrary::descriptorSetLayout() const  { return _shaderLibrary->descriptorSetLayout(); }
inline const std::vector<vk::DescriptorSetLayout>& PipelineLibrary::descriptorSetLayoutList() const  { return _shaderLibrary->descriptorSetLayoutList(); }

}
