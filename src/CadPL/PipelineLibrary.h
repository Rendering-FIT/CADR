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
	std::vector<vk::SpecializationMapEntry> specializationMap;
	std::vector<uint8_t> specializationData;
	vk::Viewport viewport;
	vk::Rect2D scissor;
	vk::CullModeFlagBits cullMode;
	vk::FrontFace frontFace;
	bool depthBiasEnable;
	float depthBiasConstant;
	float depthBiasClamp;
	float depthBiasSlopeFactor;
	float lineWidth;
	vk::SampleCountFlagBits rasterizationSamples;
	bool sampleShadingEnable;
	float minSampleShading;
	bool depthTestEnable;
	bool depthWriteEnable;

	struct BlendState {
		bool blendEnable;
		vk::BlendFactor srcColorBlendFactor;
		vk::BlendFactor dstColorBlendFactor;
		vk::BlendOp colorBlendOp;
		vk::BlendFactor srcAlphaBlendFactor;
		vk::BlendFactor dstAlphaBlendFactor;
		vk::BlendOp alphaBlendOp;
		vk::ColorComponentFlags colorWriteMask;
	};
	std::vector<BlendState> blendState;

	vk::RenderPass renderPass;
	uint32_t subpass;

	bool operator<(const PipelineState& rhs) const  { return false; }  // FIXME: This is not complete
};


class CADPL_EXPORT SharedPipeline : public CadR::Pipeline {
protected:
	void* _owner = nullptr;
public:

	SharedPipeline() = default;
	~SharedPipeline() noexcept;

	SharedPipeline(SharedPipeline&& other) noexcept;
	SharedPipeline(const SharedPipeline& other) noexcept;
	SharedPipeline& operator=(SharedPipeline&& rhs) noexcept;
	SharedPipeline& operator=(const SharedPipeline& rhs) noexcept;

	void reset() noexcept;

protected:
	friend PipelineFamily;
	SharedPipeline(void* pipelineOwner) noexcept;
};


class CADPL_EXPORT PipelineFamily {
protected:

	struct PipelineOwner {
		size_t referenceCounter;
		vk::Pipeline pipeline;
		PipelineFamily* pipelineFamily;
		std::map<PipelineState, PipelineOwner>::iterator eraseIt;
	};

	std::map<PipelineState, PipelineOwner> _pipelineMap;
	vk::PrimitiveTopology _primitiveTopology;

	static void refPipeline(void* pipelineOwner) noexcept;
	static CadR::Pipeline refAndGetPipeline(void* pipelineOwner) noexcept;
	static void unrefPipeline(void* pipelineOwner) noexcept;
	static void destroyPipeline(void* pipelineOwner) noexcept;
	vk::Pipeline createPipeline(const PipelineState& pipelineState);
	friend SharedPipeline;
	friend PipelineLibrary;

	CadR::VulkanDevice* _device = nullptr;
	PipelineLibrary* _pipelineLibrary;
	std::map<ShaderState, PipelineFamily>::iterator _eraseIt;

	SharedShaderModule _vertexShader;
	SharedShaderModule _geometryShader;
	SharedShaderModule _fragmentShader;

public:

	SharedPipeline getOrCreatePipeline(const PipelineState& pipelineState);
	SharedPipeline getPipeline(const PipelineState& pipelineState);
	const std::map<PipelineState, PipelineOwner>& pipelineMap() const;
	PipelineFamily(PipelineLibrary& pipelineLibrary) noexcept;
	~PipelineFamily() noexcept;

};


class CADPL_EXPORT PipelineLibrary {
protected:

	CadR::VulkanDevice* _device;
	ShaderLibrary* _shaderLibrary;
	std::map<ShaderState, PipelineFamily> _pipelineFamilyMap;
	vk::PipelineCache _pipelineCache;

	friend PipelineFamily;

public:

	// construction and destruction
	PipelineLibrary(ShaderLibrary& shaderLibrary, vk::PipelineCache pipelineCache = nullptr);
	~PipelineLibrary() noexcept;

	// synchronous API to get and create pipelines
	SharedPipeline getOrCreatePipeline(const ShaderState& shaderState, const PipelineState& pipelineState);
	SharedPipeline getPipeline(const ShaderState& shaderState, const PipelineState& pipelineState);

	// getters
	CadR::VulkanDevice& device() const;
	ShaderLibrary& shaderLibrary() const;
	vk::PipelineCache pipelineCache() const;

};


// inline functions
inline SharedPipeline::SharedPipeline(void* pipelineOwner) noexcept  : CadR::Pipeline(PipelineFamily::refAndGetPipeline(pipelineOwner)), _owner(pipelineOwner) {}
inline SharedPipeline::~SharedPipeline() noexcept  { if(_pipeline) PipelineFamily::unrefPipeline(_owner); }
inline SharedPipeline::SharedPipeline(SharedPipeline&& other) noexcept  : CadR::Pipeline(std::move(other)) { _owner=other._owner; other._pipeline=nullptr; }
inline SharedPipeline::SharedPipeline(const SharedPipeline& other) noexcept  : CadR::Pipeline(other) { _owner=other._owner; if(_pipeline) PipelineFamily::refPipeline(_owner); }
inline SharedPipeline& SharedPipeline::operator=(SharedPipeline&& rhs) noexcept  { if(_pipeline) PipelineFamily::unrefPipeline(_owner); CadR::Pipeline::operator=(std::move(rhs)); _owner=rhs._owner; rhs._pipeline=nullptr; return *this; }
inline SharedPipeline& SharedPipeline::operator=(const SharedPipeline& rhs) noexcept  { if(_pipeline) PipelineFamily::unrefPipeline(_owner); CadR::Pipeline::operator=(rhs); _owner=rhs._owner; if(_pipeline) PipelineFamily::refPipeline(_owner); return *this; }
inline void SharedPipeline::reset() noexcept  { if(!_pipeline) return; PipelineFamily::unrefPipeline(_owner); _pipeline=nullptr; }
inline void PipelineFamily::refPipeline(void* pipelineOwner) noexcept  { PipelineOwner* po=reinterpret_cast<PipelineOwner*>(pipelineOwner); po->referenceCounter++; }
inline CadR::Pipeline PipelineFamily::refAndGetPipeline(void* pipelineOwner) noexcept  { PipelineOwner* po=reinterpret_cast<PipelineOwner*>(pipelineOwner); po->referenceCounter++; ShaderLibrary* sl=po->pipelineFamily->_pipelineLibrary->_shaderLibrary; return CadR::Pipeline(po->pipeline, sl->pipelineLayout(), sl->descriptorSetLayoutList()); }
inline void PipelineFamily::unrefPipeline(void* pipelineOwner) noexcept  { PipelineOwner* po=reinterpret_cast<PipelineOwner*>(pipelineOwner); if(po->referenceCounter==1) PipelineFamily::destroyPipeline(pipelineOwner); else po->referenceCounter--; }
inline SharedPipeline PipelineFamily::getPipeline(const PipelineState& pipelineState)  { auto it=_pipelineMap.find(pipelineState); return (it!=_pipelineMap.end()) ? SharedPipeline(&it->second.pipeline) : SharedPipeline(); }
inline const std::map<PipelineState, PipelineFamily::PipelineOwner>& PipelineFamily::pipelineMap() const  { return _pipelineMap; }
inline PipelineLibrary::PipelineLibrary(ShaderLibrary& shaderLibrary, vk::PipelineCache pipelineCache)  : _device(&shaderLibrary.device()), _pipelineCache(pipelineCache) {}
inline SharedPipeline PipelineLibrary::getPipeline(const ShaderState& shaderState, const PipelineState& pipelineState)  { auto it=_pipelineFamilyMap.find(shaderState); return (it!=_pipelineFamilyMap.end()) ? it->second.getPipeline(pipelineState) : SharedPipeline(); }
inline CadR::VulkanDevice& PipelineLibrary::device() const  { return *_device; }
inline ShaderLibrary& PipelineLibrary::shaderLibrary() const  { return *_shaderLibrary; }
inline vk::PipelineCache PipelineLibrary::pipelineCache() const  { return _pipelineCache; }


}
