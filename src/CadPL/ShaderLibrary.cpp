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



void ShaderLibrary::destroy() noexcept
{
	if(_device) {
		_device->destroy(_pipelineLayout);
		_device->destroy(_descriptorSetLayout);
		_pipelineLayout = nullptr;
		_descriptorSetLayout = nullptr;
	}
}


ShaderLibrary::~ShaderLibrary() noexcept
{
	assert(_vertexShaderMap.empty() && "ShaderLibrary::~ShaderLibrary(): All SharedShaderModules must be released before destroying ShaderLibrary.");
	assert(_geometryShaderMap.empty() && "ShaderLibrary::~ShaderLibrary(): All SharedShaderModules must be released before destroying ShaderLibrary.");
	assert(_fragmentShaderMap.empty() && "ShaderLibrary::~ShaderLibrary(): All SharedShaderModules must be released before destroying ShaderLibrary.");

	if(_device) {
		_device->destroy(_pipelineLayout);
		_device->destroy(_descriptorSetLayout);
	}
}


ShaderLibrary::ShaderLibrary(CadR::VulkanDevice& device, uint32_t maxTextures)
	: ShaderLibrary()  // make sure thay destructor will be called when exception is thrown
{
	init(device, maxTextures);
}


void ShaderLibrary::init(CadR::VulkanDevice& device, uint32_t maxTextures)
{
	destroy();

	_device = &device;

	_descriptorSetLayout =
		_device->createDescriptorSetLayout(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,  // flags
				1,  // bindingCount
				array<vk::DescriptorSetLayoutBinding,1>{  // pBindings
					vk::DescriptorSetLayoutBinding{
						0,  // binding
						vk::DescriptorType::eCombinedImageSampler,  // descriptorType
						maxTextures, // descriptorCount
						vk::ShaderStageFlagBits::eFragment,  // stageFlags
						nullptr  // pImmutableSamplers
					}
				}.data()
			).setPNext(
				&(const vk::DescriptorSetLayoutBindingFlagsCreateInfo&)vk::DescriptorSetLayoutBindingFlagsCreateInfo(
					1,  // bindingCount
					array<vk::DescriptorBindingFlags,1>{  // pBindingFlags
						vk::DescriptorBindingFlagBits::eUpdateAfterBind |
							vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending |
							vk::DescriptorBindingFlagBits::ePartiallyBound |
							vk::DescriptorBindingFlagBits::eVariableDescriptorCount
					}.data()
				)
			)
		);
	_pipelineLayout =
		_device->createPipelineLayout(
			vk::PipelineLayoutCreateInfo(
				vk::PipelineLayoutCreateFlags(),  // flags
				1,  // setLayoutCount
				&_descriptorSetLayout,  // pSetLayouts
				1,  // pushConstantRangeCount
				array{
					vk::PushConstantRange{  // pPushConstantRanges
						vk::ShaderStageFlagBits::eAllGraphics,  // stageFlags
						0,  // offset
						56  // size
					},
				}.data()
			)
		);
	_descriptorSetLayoutList.reserve(1);
	_descriptorSetLayoutList.push_back(_descriptorSetLayout);
}


void ShaderLibrary::destroyShaderModule(void* shaderModuleObject) noexcept
{
	struct SMO {
		size_t counter;
		vk::ShaderModule sm;
		ShaderLibrary* shaderLibrary;
		OwningMap owningMap;
	};
	SMO* s = reinterpret_cast<SMO*>(shaderModuleObject); 

	s->shaderLibrary->_device->destroy(s->sm);

	switch(s->owningMap) {
	case OwningMap::eVertex:   s->shaderLibrary->_vertexShaderMap.erase(reinterpret_cast<ShaderModuleObject<VertexShaderMapKey>*>(shaderModuleObject)->eraseIt); break;
	case OwningMap::eGeometry: s->shaderLibrary->_geometryShaderMap.erase(reinterpret_cast<ShaderModuleObject<GeometryShaderMapKey>*>(shaderModuleObject)->eraseIt); break;
	case OwningMap::eFragment: s->shaderLibrary->_fragmentShaderMap.erase(reinterpret_cast<ShaderModuleObject<FragmentShaderMapKey>*>(shaderModuleObject)->eraseIt); break;
	default:
		assert(0 && "ShaderModuleObject::owningMap contains unknown value.");
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
		it->second.owningMap = OwningMap::eVertex;
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
		it->second.owningMap = OwningMap::eGeometry;
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
		it->second.owningMap = OwningMap::eFragment;
		it->second.eraseIt = it;
	}
	return SharedShaderModule(&it->second);
}
