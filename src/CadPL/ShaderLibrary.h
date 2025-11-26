#pragma once

#include <bitset>
#include <map>
#include <vulkan/vulkan.hpp>

namespace CadR {
class VulkanDevice;
}

namespace CadPL {


struct CADPL_EXPORT ShaderState {

	bool idBuffer;
	vk::PrimitiveTopology primitiveTopology;

	static constexpr const unsigned maxNumAttribs = 16;
	uint16_t attribAccessInfo[maxNumAttribs];
	uint8_t numAttribs;
	uint32_t materialSetup;
	uint16_t lightSetup[4];
	uint16_t numLights;
	uint32_t textureSetup[6];
	uint16_t numTextures;

	static constexpr const unsigned numOptimizeFlags = 7;
	static constexpr const std::bitset<numOptimizeFlags> OptimizeNone = 0x00;  //< No optimizations. Uber-shader will be used.
	static constexpr const std::bitset<numOptimizeFlags> OptimizeAttribs = 0x01;  //< Optimize attribute access. Number of attributes, their indices, their type and data offset are fixed and hardcoded into the shader code. 
	static constexpr const std::bitset<numOptimizeFlags> OptimizeMaterialModel = 0x02;  //< Optimize material model (unlit, phong, metallic-roughness,...). The material model is fixed and hardcoded into the shader code.
	static constexpr const std::bitset<numOptimizeFlags> OptimizeMaterialColorAttribute = 0x04;  //< Optimize phong color attribute settings (color to diffuse, color to ambient and diffuse). The settings are fixed and hardcoded into the shader code.
	static constexpr const std::bitset<numOptimizeFlags> OptimizeMaterialAlpha = 0x08;  //< Optimize alpha computation (ignore texture alpha, ignore material alpha, ignore color attribute alpha). The alpha flags are fixed and hardcoded into the shader code.
	static constexpr const std::bitset<numOptimizeFlags> OptimizeMaterial = 0x0e;  //< Optimize all material related settings. The settings are fixed and hardcoded into the shader code.
	static constexpr const std::bitset<numOptimizeFlags> OptimizeTextureTypesAndTexCoordIndices = 0x10;  //< Optimize texture types and attribute indices from which texture coordinates are sourced. Number of textures, their types (normal texture, occlusion texture, emissive texture, base texture,...) and attribute indices for sourcing texture coordinates are fixed and hardcoded into the shader code.
	static constexpr const std::bitset<numOptimizeFlags> OptimizeTextureFlags = 0x20;  //< Optimize texture flags (apply strength, apply texture coordinate transformation, blend color included, phong's texture environment, first component index). The settings are fixed and hardcoded into the shader code.
	static constexpr const std::bitset<numOptimizeFlags> OptimizeTextures = 0x30;  //< Optimize all texture related settings. The settings are fixed and hardcoded into the shader code.
	static constexpr const std::bitset<numOptimizeFlags> OptimizeLightTypes = 0x40;  //< Optimize light types. Number of lights and their types are fixed and hardcoded into the shader code.
	static constexpr const std::bitset<numOptimizeFlags> OptimizeLights = 0x40;  //< Optimize all light related settings.
	static constexpr const std::bitset<numOptimizeFlags> OptimizeAll = 0x7f;  //< Make all available optimizations.

	std::bitset<numOptimizeFlags> optimizeFlags = OptimizeNone;

	bool operator<(const ShaderState& rhs) const  { return idBuffer < rhs.idBuffer; }

};


class CADPL_EXPORT SharedShaderModule {
protected:
	vk::ShaderModule _shaderModule;
	void* _smObject = nullptr;
public:

	SharedShaderModule() = default;
	~SharedShaderModule() noexcept;

	SharedShaderModule(SharedShaderModule&& other) noexcept;
	SharedShaderModule(const SharedShaderModule& other) noexcept;
	SharedShaderModule& operator=(SharedShaderModule&& rhs) noexcept;
	SharedShaderModule& operator=(const SharedShaderModule& rhs) noexcept;

	vk::ShaderModule get() const;
	operator vk::ShaderModule() const;
	explicit operator bool() const;
	void reset() noexcept;

protected:
	friend class ShaderLibrary;
	SharedShaderModule(void* shaderModuleObject) noexcept;
	SharedShaderModule(vk::ShaderModule shaderModule, void* shaderModuleObject) noexcept;
};


class CADPL_EXPORT ShaderLibrary {
protected:

	CadR::VulkanDevice* _device = nullptr;

	enum class OwningMap { eUnknown = 0, eVertex, eGeometry, eFragment };

	template<typename MapKey>
	struct ShaderModuleObject {
		size_t referenceCounter;  //< Reference counter. It must be on the beginning of this structure because of implementation of some functions in this class.
		vk::ShaderModule shaderModule;  //< Shader module handle. It must be on the second place in this structure because of implementation of some functions in this class.
		ShaderLibrary* shaderLibrary;  //< ShaderLibrary owning this ShaderModuleObject. It must be on the third place in this structure because of implementation of some functions in this class.
		OwningMap owningMap;  //< It indicates the map that this structure is member of. It must be on the fourth place in this structure because of implementation of some functions in this class.
		typename std::map<MapKey,ShaderModuleObject<MapKey>>::iterator eraseIt;  //< Iterator for removing this object from the map when the referenceCounter reaches zero.
	};

	struct VertexShaderMapKey {
		bool idBuffer;
		VertexShaderMapKey(const ShaderState& shaderState);
		bool operator<(const VertexShaderMapKey& rhs) const  { return idBuffer < rhs.idBuffer; }
	};
	struct GeometryShaderMapKey {
		bool idBuffer;
		GeometryShaderMapKey(const ShaderState& shaderState);
		bool operator<(const GeometryShaderMapKey& rhs) const  { return idBuffer < rhs.idBuffer; }
	};
	struct FragmentShaderMapKey {
		bool idBuffer;
		FragmentShaderMapKey(const ShaderState& shaderState);
		bool operator<(const FragmentShaderMapKey& rhs) const  { return idBuffer < rhs.idBuffer; }
	};
	
	std::map<VertexShaderMapKey, ShaderModuleObject<VertexShaderMapKey>> _vertexShaderMap;
	std::map<GeometryShaderMapKey, ShaderModuleObject<GeometryShaderMapKey>> _geometryShaderMap;
	std::map<FragmentShaderMapKey, ShaderModuleObject<FragmentShaderMapKey>> _fragmentShaderMap;
	vk::PipelineLayout _pipelineLayout;
	vk::DescriptorSetLayout _descriptorSetLayout;
	std::vector<vk::DescriptorSetLayout> _descriptorSetLayoutList;
	
	static void refShaderModule(void* shaderModuleObject) noexcept;
	static vk::ShaderModule refAndGetShaderModule(void* shaderModuleObject) noexcept;
	static void unrefShaderModule(void* shaderModuleObject) noexcept;
	static void destroyShaderModule(void* shaderModuleObject) noexcept;
	friend class SharedShaderModule;

public:

	// construction and destruction
	ShaderLibrary() noexcept = default;
	ShaderLibrary(CadR::VulkanDevice& device, uint32_t maxTextures = 250000);
	~ShaderLibrary() noexcept;
	void init(CadR::VulkanDevice& device, uint32_t maxTextures = 250000);
	void destroy() noexcept;

	// synchronous API to get and create pipelines
	SharedShaderModule getOrCreateVertexShader(const ShaderState& state);
	SharedShaderModule getOrCreateGeometryShader(const ShaderState& state);
	SharedShaderModule getOrCreateFragmentShader(const ShaderState& state);
	SharedShaderModule getVertexShader(const ShaderState& state);
	SharedShaderModule getGeometryShader(const ShaderState& state);
	SharedShaderModule getFragmentShader(const ShaderState& state);

	// getters
	CadR::VulkanDevice& device() const;
	vk::PipelineLayout pipelineLayout() const;
	vk::DescriptorSetLayout descriptorSetLayout() const;
	std::vector<vk::DescriptorSetLayout>* descriptorSetLayoutList();
	const std::vector<vk::DescriptorSetLayout>* descriptorSetLayoutList() const;

};


// inline functions
inline SharedShaderModule::SharedShaderModule(void* shaderModuleObject) noexcept  : _shaderModule(ShaderLibrary::refAndGetShaderModule(shaderModuleObject)), _smObject(shaderModuleObject) {}
inline SharedShaderModule::SharedShaderModule(vk::ShaderModule shaderModule, void* shaderModuleObject) noexcept  : _shaderModule(shaderModule), _smObject(shaderModuleObject) { if(shaderModuleObject) ShaderLibrary::refShaderModule(shaderModuleObject); }
inline SharedShaderModule::~SharedShaderModule() noexcept  { if(_shaderModule) ShaderLibrary::unrefShaderModule(_smObject); }
inline SharedShaderModule::SharedShaderModule(SharedShaderModule&& other) noexcept  : _shaderModule(other._shaderModule), _smObject(other._smObject) { other._shaderModule=nullptr; }
inline SharedShaderModule::SharedShaderModule(const SharedShaderModule& other) noexcept  : _shaderModule(other._shaderModule), _smObject(other._smObject) { if(_smObject) ShaderLibrary::refShaderModule(_smObject); }
inline SharedShaderModule& SharedShaderModule::operator=(SharedShaderModule&& rhs) noexcept  { if(_shaderModule) ShaderLibrary::unrefShaderModule(_smObject); _shaderModule=rhs._shaderModule; _smObject=rhs._smObject; rhs._shaderModule=nullptr; return *this; }
inline SharedShaderModule& SharedShaderModule::operator=(const SharedShaderModule& rhs) noexcept  { if(_shaderModule) ShaderLibrary::unrefShaderModule(_smObject); _shaderModule=rhs._shaderModule; _smObject=rhs._smObject; if(_shaderModule) ShaderLibrary::refShaderModule(_smObject); return *this; }
inline vk::ShaderModule SharedShaderModule::get() const  { return _shaderModule; }
inline SharedShaderModule::operator vk::ShaderModule() const  { return _shaderModule; }
inline SharedShaderModule::operator bool() const  { return _smObject; }
inline void SharedShaderModule::reset() noexcept  { if(!_smObject) return; ShaderLibrary::unrefShaderModule(_smObject); _shaderModule=nullptr; _smObject=nullptr; }

inline ShaderLibrary::VertexShaderMapKey::VertexShaderMapKey(const ShaderState& shaderState)  : idBuffer(shaderState.idBuffer) {}
inline ShaderLibrary::GeometryShaderMapKey::GeometryShaderMapKey(const ShaderState& shaderState)  : idBuffer(shaderState.idBuffer) {}
inline ShaderLibrary::FragmentShaderMapKey::FragmentShaderMapKey(const ShaderState& shaderState)  : idBuffer(shaderState.idBuffer) {}
inline void ShaderLibrary::refShaderModule(void* shaderModuleObject) noexcept  { size_t& counter=*reinterpret_cast<size_t*>(shaderModuleObject); counter++; }
inline vk::ShaderModule ShaderLibrary::refAndGetShaderModule(void* shaderModuleObject) noexcept  { struct SMO { size_t counter; vk::ShaderModule sm; }; SMO* s=reinterpret_cast<SMO*>(shaderModuleObject); s->counter++; return s->sm; }
inline void ShaderLibrary::unrefShaderModule(void* shaderModuleObject) noexcept  { size_t& counter=*reinterpret_cast<size_t*>(shaderModuleObject); if(counter==1) ShaderLibrary::destroyShaderModule(shaderModuleObject); else counter--; }
inline SharedShaderModule ShaderLibrary::getVertexShader(const ShaderState& state)  { auto it=_vertexShaderMap.find(state); return (it!=_vertexShaderMap.end()) ? SharedShaderModule(&it->second) : SharedShaderModule(); }
inline SharedShaderModule ShaderLibrary::getGeometryShader(const ShaderState& state)  { auto it=_geometryShaderMap.find(state); return (it!=_geometryShaderMap.end()) ? SharedShaderModule(&it->second) : SharedShaderModule(); }
inline SharedShaderModule ShaderLibrary::getFragmentShader(const ShaderState& state)  { auto it=_fragmentShaderMap.find(state); return (it!=_fragmentShaderMap.end()) ? SharedShaderModule(&it->second) : SharedShaderModule(); }
inline CadR::VulkanDevice& ShaderLibrary::device() const  { return *_device; }
inline vk::PipelineLayout ShaderLibrary::pipelineLayout() const  { return _pipelineLayout; }
inline vk::DescriptorSetLayout ShaderLibrary::descriptorSetLayout() const  { return _descriptorSetLayout; }
inline std::vector<vk::DescriptorSetLayout>* ShaderLibrary::descriptorSetLayoutList()  { return &_descriptorSetLayoutList; }
inline const std::vector<vk::DescriptorSetLayout>* ShaderLibrary::descriptorSetLayoutList() const  { return &_descriptorSetLayoutList; }


}
