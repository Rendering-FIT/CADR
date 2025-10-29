#include <CadPL/ShaderLibrary.h>
#include <CadPL/ShaderGenerator.h>
#include <CadR/VulkanDevice.h>

using namespace std;
using namespace CadPL;

// shader code in SPIR-V binary
static const uint32_t vertexUberShaderSpirv[]={
#include "shaders/UberShader.vert.spv"
};
static const uint32_t vertexIdBufferUberShaderSpirv[]={
#include "shaders/UberShader-idBuffer.vert.spv"
};
static const uint32_t geometryUberShaderSpirv[]={
#include "shaders/UberShader.geom.spv"
};
static const uint32_t geometryIdBufferUberShaderSpirv[]={
#include "shaders/UberShader-idBuffer.geom.spv"
};
static const uint32_t fragmentUberShaderSpirv[]={
#include "shaders/UberShader.frag.spv"
};
static const uint32_t fragmentIdBufferUberShaderSpirv[]={
#include "shaders/UberShader-idBuffer.frag.spv"
};



ShaderLibrary::~ShaderLibrary()
{
	assert(_vertexShaderMap.empty() && "ShaderLibrary::~ShaderLibrary(): All SharedShaderModules must be released before destroying ShaderLibrary.");
	assert(_geometryShaderMap.empty() && "ShaderLibrary::~ShaderLibrary(): All SharedShaderModules must be released before destroying ShaderLibrary.");
	assert(_fragmentShaderMap.empty() && "ShaderLibrary::~ShaderLibrary(): All SharedShaderModules must be released before destroying ShaderLibrary.");
}


ShaderLibrary::ShaderLibrary(CadR::VulkanDevice& device) noexcept
	: _device(&device)
{
}


void ShaderLibrary::destroyShaderModule(void* shaderModuleOwner) noexcept
{
	struct SMO {
		size_t counter;
		vk::ShaderModule sm;
		ShaderLibrary* shaderLibrary;
		OwnerMap ownerMap;
	};
	SMO* s = reinterpret_cast<SMO*>(shaderModuleOwner); 

	s->shaderLibrary->_device->destroy(s->sm);

	switch(s->ownerMap) {
	case OwnerMap::eVertex:   s->shaderLibrary->_vertexShaderMap.erase(reinterpret_cast<ShaderModuleOwner<VertexShaderMapKey>*>(shaderModuleOwner)->eraseIt); break;
	case OwnerMap::eGeometry: s->shaderLibrary->_geometryShaderMap.erase(reinterpret_cast<ShaderModuleOwner<GeometryShaderMapKey>*>(shaderModuleOwner)->eraseIt); break;
	case OwnerMap::eFragment: s->shaderLibrary->_fragmentShaderMap.erase(reinterpret_cast<ShaderModuleOwner<FragmentShaderMapKey>*>(shaderModuleOwner)->eraseIt); break;
	default:
		assert(0 && "ShaderModuleOwner::ownerMapIndex contains unhandled value.");
	}
}


SharedShaderModule ShaderLibrary::getOrCreateVertexShader(const ShaderState& state)
{
	VertexShaderMapKey key(state);
	auto [it, newRecord] = _vertexShaderMap.try_emplace(key);
	if(newRecord) {
		try {
			it->second.shaderModule = ShaderGenerator::createVertexShader(state, *_device);
		} catch(...) {
			_vertexShaderMap.erase(it);
			throw;
		}
		it->second.referenceCounter = 0;
		it->second.shaderLibrary = this;
		it->second.ownerMap = OwnerMap::eVertex;
		it->second.eraseIt = it;
	}
	return SharedShaderModule(&it->second);
}


SharedShaderModule ShaderLibrary::getOrCreateGeometryShader(const ShaderState& state)
{
	GeometryShaderMapKey key(state);
	auto [it, newRecord] = _geometryShaderMap.try_emplace(key);
	if(newRecord) {
		try {
			it->second.shaderModule = ShaderGenerator::createGeometryShader(state, *_device);
		} catch(...) {
			_geometryShaderMap.erase(it);
			throw;
		}
		it->second.referenceCounter = 0;
		it->second.shaderLibrary = this;
		it->second.ownerMap = OwnerMap::eGeometry;
		it->second.eraseIt = it;
	}
	return SharedShaderModule(&it->second);
}


SharedShaderModule ShaderLibrary::getOrCreateFragmentShader(const ShaderState& state)
{
	FragmentShaderMapKey key(state);
	auto [it, newRecord] = _fragmentShaderMap.try_emplace(key);
	if(newRecord) {
		try {
			it->second.shaderModule = ShaderGenerator::createFragmentShader(state, *_device);
		} catch(...) {
			_fragmentShaderMap.erase(it);
			throw;
		}
		it->second.referenceCounter = 0;
		it->second.shaderLibrary = this;
		it->second.ownerMap = OwnerMap::eFragment;
		it->second.eraseIt = it;
	}
	return SharedShaderModule(&it->second);
}
