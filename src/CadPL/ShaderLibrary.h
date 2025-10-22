#pragma once

#include <bitset>
#include <map>
#include <vulkan/vulkan.hpp>
#include <CadR/VulkanDevice.h>

namespace CadR {
class VulkanDevice;
}

namespace CadPL {


struct ShaderState {

	bool idBuffer;

	static constexpr const unsigned maxNumAttribs = 16;
	uint16_t attribAccessInfo[maxNumAttribs];
	uint8_t numAttribs;
	uint32_t materialSetup;
	uint16_t lightSetup[4];
	uint16_t numLights;
	uint32_t textureSetup[6];
	uint16_t numTextures;

	static constexpr const unsigned numOptimizeFlags = 8;
	std::bitset<numOptimizeFlags> optimizeFlags = 0;
	static constexpr const std::bitset<numOptimizeFlags> OptimizeAttribs = 0x01;
	static constexpr const std::bitset<numOptimizeFlags> OptimizeMaterialModel = 0x02;
	static constexpr const std::bitset<numOptimizeFlags> OptimizeMaterialColorAttribute = 0x04;
	static constexpr const std::bitset<numOptimizeFlags> OptimizeMaterialAlpha = 0x08;
	static constexpr const std::bitset<numOptimizeFlags> OptimizeMaterial = 0x0e;
	static constexpr const std::bitset<numOptimizeFlags> OptimizeTextureFlags = 0x10;
	static constexpr const std::bitset<numOptimizeFlags> OptimizeTextureCoordAccess = 0x20;
	static constexpr const std::bitset<numOptimizeFlags> OptimizeTextureTypes = 0x40;
	static constexpr const std::bitset<numOptimizeFlags> OptimizeTextures = 0x70;
	static constexpr const std::bitset<numOptimizeFlags> OptimizeLightType = 0x80;
	static constexpr const std::bitset<numOptimizeFlags> OptimizeLights = 0x80;
	static constexpr const std::bitset<numOptimizeFlags> OptimizeAll = 0xff;

	bool operator<(const ShaderState& rhs) const  { return idBuffer < rhs.idBuffer; }
};


class CADPL_EXPORT ShaderGenerator {
public:
	static [[nodiscard]] vk::ShaderModule createVertexShader(const ShaderState& state, CadR::VulkanDevice& device);
	static [[nodiscard]] vk::ShaderModule createGeometryShader(const ShaderState& state, CadR::VulkanDevice& device);
	static [[nodiscard]] vk::ShaderModule createFragmentShader(const ShaderState& state, CadR::VulkanDevice& device);
	static vk::UniqueHandle<vk::ShaderModule, CadR::VulkanDevice> createVertexShaderUnique(const ShaderState& state, CadR::VulkanDevice& device);
	static vk::UniqueHandle<vk::ShaderModule, CadR::VulkanDevice> createGeometryShaderUnique(const ShaderState& state, CadR::VulkanDevice& device);
	static vk::UniqueHandle<vk::ShaderModule, CadR::VulkanDevice> createFragmentShaderUnique(const ShaderState& state, CadR::VulkanDevice& device);
};


class CADPL_EXPORT SharedShaderModule {
protected:
	vk::ShaderModule _shaderModule;
	void* _owner = nullptr;
public:

	SharedShaderModule() = default;
	~SharedShaderModule() noexcept;

	SharedShaderModule(SharedShaderModule&& other) noexcept;
	SharedShaderModule(const SharedShaderModule& other) noexcept;
	SharedShaderModule& operator=(SharedShaderModule&& rhs) noexcept;
	SharedShaderModule& operator=(const SharedShaderModule& rhs) noexcept;

	vk::ShaderModule get() const;
	operator vk::ShaderModule() const;
	void reset() noexcept;

protected:
	friend class ShaderLibrary;
	SharedShaderModule(void* shaderLibraryModuleOwner) noexcept;
	SharedShaderModule(vk::ShaderModule shaderModule, void* shaderLibraryModuleOwner) noexcept;
};


class CADPL_EXPORT ShaderLibrary {
protected:
	CadR::VulkanDevice* _device;

	enum class OwnerMap { eUnknown = 0, eVertex, eGeometry, eFragment };

	template<typename MapKey>
	struct ShaderModuleOwner {
		size_t referenceCounter;
		vk::ShaderModule shaderModule;
		ShaderLibrary* shaderLibrary;
		OwnerMap ownerMap;
		typename std::map<MapKey,ShaderModuleOwner<MapKey>>::iterator eraseIt;
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
	
	std::map<VertexShaderMapKey, ShaderModuleOwner<VertexShaderMapKey>> _vertexShaderMap;
	std::map<GeometryShaderMapKey, ShaderModuleOwner<GeometryShaderMapKey>> _geometryShaderMap;
	std::map<FragmentShaderMapKey, ShaderModuleOwner<FragmentShaderMapKey>> _fragmentShaderMap;
	
	static void refShaderModule(void* shaderModuleOwner) noexcept;
	static vk::ShaderModule refAndGetShaderModule(void* shaderModuleOwner) noexcept;
	static void unrefShaderModule(void* shaderModuleOwner) noexcept;
	static void destroyShaderModule(void* shaderModuleOwner) noexcept;
	friend class SharedShaderModule;

public:

	ShaderLibrary(CadR::VulkanDevice& device) noexcept;
	~ShaderLibrary() noexcept;

	SharedShaderModule getVertexShader(const ShaderState& state);
	SharedShaderModule getGeometryShader(const ShaderState& state);
	SharedShaderModule getFragmentShader(const ShaderState& state);

	CadR::VulkanDevice& device() const;

};


// inline functions
inline vk::UniqueHandle<vk::ShaderModule, CadR::VulkanDevice> ShaderGenerator::createVertexShaderUnique(const ShaderState& state, CadR::VulkanDevice& device)  { return vk::UniqueHandle<vk::ShaderModule, CadR::VulkanDevice>(createVertexShader(state, device)); }
inline vk::UniqueHandle<vk::ShaderModule, CadR::VulkanDevice> ShaderGenerator::createGeometryShaderUnique(const ShaderState& state, CadR::VulkanDevice& device)  { return vk::UniqueHandle<vk::ShaderModule, CadR::VulkanDevice>(createGeometryShader(state, device)); }
inline vk::UniqueHandle<vk::ShaderModule, CadR::VulkanDevice> ShaderGenerator::createFragmentShaderUnique(const ShaderState& state, CadR::VulkanDevice& device)  { return vk::UniqueHandle<vk::ShaderModule, CadR::VulkanDevice>(createFragmentShader(state, device)); }
inline SharedShaderModule::SharedShaderModule(void* shaderLibraryModuleOwner) noexcept  : _shaderModule(ShaderLibrary::refAndGetShaderModule(shaderLibraryModuleOwner)), _owner(shaderLibraryModuleOwner) {}
inline SharedShaderModule::SharedShaderModule(vk::ShaderModule shaderModule, void* shaderLibraryModuleOwner) noexcept  : _shaderModule(shaderModule), _owner(shaderLibraryModuleOwner) { if(shaderModule) ShaderLibrary::refShaderModule(shaderLibraryModuleOwner); }
inline SharedShaderModule::~SharedShaderModule() noexcept  { if(_shaderModule) ShaderLibrary::unrefShaderModule(_owner); }
inline SharedShaderModule::SharedShaderModule(SharedShaderModule&& other) noexcept  { if(_shaderModule) ShaderLibrary::unrefShaderModule(_owner); _shaderModule=other._shaderModule; _owner=other._owner; other._shaderModule=nullptr; }
inline SharedShaderModule::SharedShaderModule(const SharedShaderModule& other) noexcept  { if(_shaderModule) ShaderLibrary::unrefShaderModule(_owner); _shaderModule=other._shaderModule; _owner=other._owner; if(_shaderModule) ShaderLibrary::refShaderModule(_owner); }
inline SharedShaderModule& SharedShaderModule::operator=(SharedShaderModule&& rhs) noexcept  { if(_shaderModule) ShaderLibrary::unrefShaderModule(_owner); _shaderModule=rhs._shaderModule; _owner=rhs._owner; rhs._shaderModule=nullptr; return *this; }
inline SharedShaderModule& SharedShaderModule::operator=(const SharedShaderModule& rhs) noexcept  { if(_shaderModule) ShaderLibrary::unrefShaderModule(_owner); _shaderModule=rhs._shaderModule; _owner=rhs._owner; if(_shaderModule) ShaderLibrary::refShaderModule(_owner); return *this; }
inline vk::ShaderModule SharedShaderModule::get() const  { return _shaderModule; }
inline SharedShaderModule::operator vk::ShaderModule() const  { return _shaderModule; }
inline void SharedShaderModule::reset() noexcept  { if(_shaderModule==nullptr) return; ShaderLibrary::unrefShaderModule(_owner); _shaderModule=nullptr; }
inline ShaderLibrary::VertexShaderMapKey::VertexShaderMapKey(const ShaderState& shaderState)  : idBuffer(shaderState.idBuffer) {}
inline ShaderLibrary::GeometryShaderMapKey::GeometryShaderMapKey(const ShaderState& shaderState)  : idBuffer(shaderState.idBuffer) {}
inline ShaderLibrary::FragmentShaderMapKey::FragmentShaderMapKey(const ShaderState& shaderState)  : idBuffer(shaderState.idBuffer) {}
inline void ShaderLibrary::refShaderModule(void* shaderModuleOwner) noexcept  { size_t& counter=reinterpret_cast<size_t&>(shaderModuleOwner); counter++; }
inline vk::ShaderModule ShaderLibrary::refAndGetShaderModule(void* shaderModuleOwner) noexcept  { struct SMO { size_t counter; vk::ShaderModule sm; }; SMO* s=reinterpret_cast<SMO*>(shaderModuleOwner); s->counter++; return s->sm; }
inline void ShaderLibrary::unrefShaderModule(void* shaderModuleOwner) noexcept  { size_t& counter=reinterpret_cast<size_t&>(shaderModuleOwner); if(counter==1) ShaderLibrary::destroyShaderModule(shaderModuleOwner); else counter--; }
inline CadR::VulkanDevice& ShaderLibrary::device() const  { return *_device; }


}
