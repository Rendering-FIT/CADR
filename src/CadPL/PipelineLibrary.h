#pragma once

#include <map>
#include <vulkan/vulkan.hpp>
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
		Value,  // viewport and scissor is specified by PipelineState struct
		DynamicState,  // viewport and scissor is specified by Vulkan dynamic state; values inside PipelineState struct is ignored
		SetFunction,  // 
	};
	ViewportAndScissorHandling viewportAndScissorHandling = ViewportAndScissorHandling::SetFunction;
	unsigned viewportAndScissorIndex = 0;
	vk::Viewport viewport;
	vk::Rect2D scissor;

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

	struct BlendState {
		bool blendEnable = false;
		vk::BlendFactor srcColorBlendFactor;
		vk::BlendFactor dstColorBlendFactor;
		vk::BlendOp colorBlendOp;
		vk::BlendFactor srcAlphaBlendFactor;
		vk::BlendFactor dstAlphaBlendFactor;
		vk::BlendOp alphaBlendOp;
		vk::ColorComponentFlags colorWriteMask;
	};
	std::vector<BlendState> blendState;

	vk::RenderPass renderPass = nullptr;
	uint32_t subpass = 0;

	bool operator<(const PipelineState& rhs) const  { return false; }  // FIXME: This is not complete

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
	const PipelineState* pipelineState() const;

protected:
	friend PipelineFamily;
	SharedPipeline(void* pipelineOwner) noexcept;
};


class CADPL_EXPORT PipelineFamily {
protected:

	CadR::VulkanDevice* _device = nullptr;
	PipelineLibrary* _pipelineLibrary;
	std::map<ShaderState, PipelineFamily>::iterator _eraseIt;

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

	vk::Pipeline createPipeline(const PipelineState& pipelineState);

	friend SharedPipeline;
	friend PipelineLibrary;

public:

	SharedPipeline getOrCreatePipeline(const PipelineState& pipelineState);
	SharedPipeline getPipeline(const PipelineState& pipelineState);
	const std::map<PipelineState, PipelineObject>& pipelineMap() const;
	PipelineFamily(PipelineLibrary& pipelineLibrary) noexcept;
	~PipelineFamily() noexcept;

};


class CADPL_EXPORT PipelineLibrary {
protected:

	CadR::VulkanDevice* _device;
	ShaderLibrary* _shaderLibrary;
	std::map<ShaderState, PipelineFamily> _pipelineFamilyMap;
	vk::PipelineCache _pipelineCache;

	std::vector<vk::Viewport> _viewportList;
	std::vector<vk::Rect2D> _scissorList;

	friend PipelineFamily;

public:

	// construction and destruction
	PipelineLibrary() noexcept = default;
	PipelineLibrary(ShaderLibrary& shaderLibrary, vk::PipelineCache pipelineCache = nullptr);
	~PipelineLibrary() noexcept;
	void init(ShaderLibrary& shaderLibrary, vk::PipelineCache pipelineCache = nullptr);

	// viewport and scissor for pipelines
	void setViewportAndScissor(const vk::Viewport& viewport, const vk::Rect2D& scissor);
	void setViewportAndScissor(const std::vector<vk::Viewport>& viewportList,
		const std::vector<vk::Rect2D>& scissorList);

	// synchronous API to get and create pipelines
	SharedPipeline getOrCreatePipeline(const ShaderState& shaderState, const PipelineState& pipelineState);
	SharedPipeline getPipeline(const ShaderState& shaderState, const PipelineState& pipelineState);

	// getters
	CadR::VulkanDevice& device() const;
	ShaderLibrary& shaderLibrary() const;
	vk::PipelineCache pipelineCache() const;
	vk::PipelineLayout pipelineLayout() const;
	vk::DescriptorSetLayout descriptorSetLayout() const;
	std::vector<vk::DescriptorSetLayout>* descriptorSetLayoutList();
	const std::vector<vk::DescriptorSetLayout>* descriptorSetLayoutList() const;

};


// inline functions
inline SharedPipeline::SharedPipeline(void* pipelineObject) noexcept  : _pipelineObject(pipelineObject) { PipelineFamily::refPipeline(pipelineObject); }
inline SharedPipeline::~SharedPipeline() noexcept  { if(_pipelineObject) PipelineFamily::unrefPipeline(_pipelineObject); }
inline SharedPipeline::SharedPipeline(SharedPipeline&& other) noexcept  : _pipelineObject(other._pipelineObject) { other._pipelineObject=nullptr; }
inline SharedPipeline::SharedPipeline(const SharedPipeline& other) noexcept  : _pipelineObject(other._pipelineObject) { if(_pipelineObject) PipelineFamily::refPipeline(_pipelineObject); }
inline SharedPipeline& SharedPipeline::operator=(SharedPipeline&& rhs) noexcept  { if(_pipelineObject) PipelineFamily::unrefPipeline(_pipelineObject); _pipelineObject=rhs._pipelineObject; rhs._pipelineObject=nullptr; return *this; }
inline SharedPipeline& SharedPipeline::operator=(const SharedPipeline& rhs) noexcept  { if(_pipelineObject) PipelineFamily::unrefPipeline(_pipelineObject); _pipelineObject=rhs._pipelineObject; if(_pipelineObject) PipelineFamily::refPipeline(_pipelineObject); return *this; }
inline void SharedPipeline::reset() noexcept  { if(!_pipelineObject) return; PipelineFamily::unrefPipeline(_pipelineObject); _pipelineObject=nullptr; }
inline const CadR::Pipeline* SharedPipeline::cadrPipeline() const  { return (_pipelineObject) ? &reinterpret_cast<PipelineFamily::PipelineObject*>(_pipelineObject)->cadrPipeline : nullptr; }
inline const PipelineState* SharedPipeline::pipelineState() const  { return (_pipelineObject) ? &reinterpret_cast<PipelineFamily::PipelineObject*>(_pipelineObject)->mapIterator->first : nullptr; }

inline void PipelineFamily::refPipeline(void* pipelineObject) noexcept  { PipelineObject* po=reinterpret_cast<PipelineObject*>(pipelineObject); po->referenceCounter++; }
inline void PipelineFamily::unrefPipeline(void* pipelineObject) noexcept  { PipelineObject* po=reinterpret_cast<PipelineObject*>(pipelineObject); if(po->referenceCounter==1) PipelineFamily::destroyPipeline(po); else po->referenceCounter--; }
inline SharedPipeline PipelineFamily::getPipeline(const PipelineState& pipelineState)  { auto it=_pipelineMap.find(pipelineState); return (it!=_pipelineMap.end()) ? SharedPipeline(&it->second) : SharedPipeline(); }
inline const std::map<PipelineState, PipelineFamily::PipelineObject>& PipelineFamily::pipelineMap() const  { return _pipelineMap; }

inline PipelineLibrary::PipelineLibrary(ShaderLibrary& shaderLibrary, vk::PipelineCache pipelineCache)  : _device(&shaderLibrary.device()), _shaderLibrary(&shaderLibrary), _pipelineCache(pipelineCache) {}
inline void PipelineLibrary::init(ShaderLibrary& shaderLibrary, vk::PipelineCache pipelineCache)  { _device=&shaderLibrary.device(); _shaderLibrary=&shaderLibrary; _pipelineCache=pipelineCache; }
inline void PipelineLibrary::setViewportAndScissor(const vk::Viewport& viewport, const vk::Rect2D& scissor)  { setViewportAndScissor(std::vector{viewport}, std::vector{scissor}); }
inline SharedPipeline PipelineLibrary::getPipeline(const ShaderState& shaderState, const PipelineState& pipelineState)  { auto it=_pipelineFamilyMap.find(shaderState); return (it!=_pipelineFamilyMap.end()) ? it->second.getPipeline(pipelineState) : SharedPipeline(); }
inline CadR::VulkanDevice& PipelineLibrary::device() const  { return *_device; }
inline ShaderLibrary& PipelineLibrary::shaderLibrary() const  { return *_shaderLibrary; }
inline vk::PipelineCache PipelineLibrary::pipelineCache() const  { return _pipelineCache; }
inline vk::PipelineLayout PipelineLibrary::pipelineLayout() const  { return _shaderLibrary->pipelineLayout(); }
inline vk::DescriptorSetLayout PipelineLibrary::descriptorSetLayout() const  { return _shaderLibrary->descriptorSetLayout(); }
inline std::vector<vk::DescriptorSetLayout>* PipelineLibrary::descriptorSetLayoutList()  { return _shaderLibrary->descriptorSetLayoutList(); }
inline const std::vector<vk::DescriptorSetLayout>* PipelineLibrary::descriptorSetLayoutList() const  { return _shaderLibrary->descriptorSetLayoutList(); }

}
