#pragma once

#include <map>
#include <vulkan/vulkan.hpp>
#include <CadPL/ShaderLibrary.h>

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
	vk::PrimitiveTopology primitiveTopology;
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


class CADPL_EXPORT SharedPipeline {
protected:
	vk::Pipeline _pipeline;
	void* _owner = nullptr;
public:

	SharedPipeline() = default;
	~SharedPipeline() noexcept;

	SharedPipeline(SharedPipeline&& other) noexcept;
	SharedPipeline(const SharedPipeline& other) noexcept;
	SharedPipeline& operator=(SharedPipeline&& rhs) noexcept;
	SharedPipeline& operator=(const SharedPipeline& rhs) noexcept;

	vk::Pipeline get() const;
	operator vk::Pipeline() const;
	void reset() noexcept;

protected:
	friend PipelineFamily;
	SharedPipeline(void* pipelineOwner) noexcept;
	SharedPipeline(vk::Pipeline pipeline, void* pipelineOwner) noexcept;
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
	
	static void refPipeline(void* pipelineOwner) noexcept;
	static vk::Pipeline refAndGetPipeline(void* pipelineOwner) noexcept;
	static void unrefPipeline(void* pipelineOwner) noexcept;
	static void destroyPipeline(void* pipelineOwner) noexcept;
	vk::Pipeline createPipeline(const PipelineState& pipelineState);
	friend SharedPipeline;

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

protected:

	void setEraseIt(std::map<ShaderState, PipelineFamily>::iterator eraseIt) noexcept;
	friend PipelineLibrary;

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
inline SharedPipeline::SharedPipeline(void* pipelineOwner) noexcept  : _pipeline(PipelineFamily::refAndGetPipeline(pipelineOwner)), _owner(pipelineOwner) {}
inline SharedPipeline::SharedPipeline(vk::Pipeline pipeline, void* pipelineOwner) noexcept  : _pipeline(pipeline), _owner(pipelineOwner) { if(pipeline) PipelineFamily::refPipeline(pipelineOwner); }
inline SharedPipeline::~SharedPipeline() noexcept  { if(_pipeline) PipelineFamily::unrefPipeline(_owner); }
inline SharedPipeline::SharedPipeline(SharedPipeline&& other) noexcept  { if(_pipeline) PipelineFamily::unrefPipeline(_owner); _pipeline=other._pipeline; _owner=other._owner; other._pipeline=nullptr; }
inline SharedPipeline::SharedPipeline(const SharedPipeline& other) noexcept  { if(_pipeline) PipelineFamily::unrefPipeline(_owner); _pipeline=other._pipeline; _owner=other._owner; if(_pipeline) PipelineFamily::refPipeline(_owner); }
inline SharedPipeline& SharedPipeline::operator=(SharedPipeline&& rhs) noexcept  { if(_pipeline) PipelineFamily::unrefPipeline(_owner); _pipeline=rhs._pipeline; _owner=rhs._owner; rhs._pipeline=nullptr; return *this; }
inline SharedPipeline& SharedPipeline::operator=(const SharedPipeline& rhs) noexcept  { if(_pipeline) PipelineFamily::unrefPipeline(_owner); _pipeline=rhs._pipeline; _owner=rhs._owner; if(_pipeline) PipelineFamily::refPipeline(_owner); return *this; }
inline vk::Pipeline SharedPipeline::get() const  { return _pipeline; }
inline SharedPipeline::operator vk::Pipeline() const  { return _pipeline; }
inline void SharedPipeline::reset() noexcept  { if(!_pipeline) return; PipelineFamily::unrefPipeline(_owner); _pipeline=nullptr; }
inline void PipelineFamily::refPipeline(void* pipelineOwner) noexcept  { size_t& p=reinterpret_cast<size_t&>(pipelineOwner); p++; }
inline vk::Pipeline PipelineFamily::refAndGetPipeline(void* pipelineOwner) noexcept  { struct PO { size_t counter; vk::Pipeline pipeline; }; PO* p=reinterpret_cast<PO*>(pipelineOwner); p->counter++; return p->pipeline; }
inline void PipelineFamily::unrefPipeline(void* pipelineOwner) noexcept  { size_t& counter=reinterpret_cast<size_t&>(pipelineOwner); if(counter==1) PipelineFamily::destroyPipeline(pipelineOwner); else counter--; }
inline SharedPipeline PipelineFamily::getPipeline(const PipelineState& pipelineState)  { auto it=_pipelineMap.find(pipelineState); return (it!=_pipelineMap.end()) ? SharedPipeline(&it->second.pipeline) : SharedPipeline(); }
inline const std::map<PipelineState, PipelineFamily::PipelineOwner>& PipelineFamily::pipelineMap() const  { return _pipelineMap; }
inline void PipelineFamily::setEraseIt(std::map<ShaderState, PipelineFamily>::iterator eraseIt) noexcept  { _eraseIt = eraseIt; }
inline PipelineLibrary::PipelineLibrary(ShaderLibrary& shaderLibrary, vk::PipelineCache pipelineCache)  : _device(&shaderLibrary.device()), _pipelineCache(pipelineCache) {}
inline SharedPipeline PipelineLibrary::getPipeline(const ShaderState& shaderState, const PipelineState& pipelineState)  { auto it=_pipelineFamilyMap.find(shaderState); return (it!=_pipelineFamilyMap.end()) ? it->second.getPipeline(pipelineState) : SharedPipeline(); }
inline CadR::VulkanDevice& PipelineLibrary::device() const  { return *_device; }
inline ShaderLibrary& PipelineLibrary::shaderLibrary() const  { return *_shaderLibrary; }
inline vk::PipelineCache PipelineLibrary::pipelineCache() const  { return _pipelineCache; }


}
