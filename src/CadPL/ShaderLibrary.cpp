#include <CadPL/ShaderLibrary.h>
#include <CadPL/ShaderGenerator.h>
#include <CadR/VulkanDevice.h>

using namespace std;
using namespace CadPL;



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
						60  // size
					},
				}.data()
			)
		);
	_descriptorSetLayoutList.reserve(1);
	_descriptorSetLayoutList.push_back(_descriptorSetLayout);
}


void ShaderLibrary::destroyShaderModule(void* shaderModuleObject) noexcept
{
	AbstractShaderModuleObject* smObject = static_cast<AbstractShaderModuleObject*>(shaderModuleObject); 

	smObject->shaderLibrary->_device->destroy(smObject->shaderModule);

	switch(smObject->owningMap) {
	case OwningMap::eVertex:   smObject->shaderLibrary->_vertexShaderMap.erase(static_cast<ShaderModuleObject<VertexShaderMapKey>*>(smObject)->eraseIt); break;
	case OwningMap::eGeometry: smObject->shaderLibrary->_geometryShaderMap.erase(static_cast<ShaderModuleObject<GeometryShaderMapKey>*>(smObject)->eraseIt); break;
	case OwningMap::eFragment: smObject->shaderLibrary->_fragmentShaderMap.erase(static_cast<ShaderModuleObject<FragmentShaderMapKey>*>(smObject)->eraseIt); break;
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


bool ShaderState::operator<(const ShaderState& rhs) const
{
	if(attribAccessInfo < rhs.attribAccessInfo)  return true;
	if(attribAccessInfo > rhs.attribAccessInfo)  return false;
	if(attribSetup < rhs.attribSetup)  return true;
	if(attribSetup > rhs.attribSetup)  return false;
	if(materialSetup < rhs.materialSetup)  return true;
	if(materialSetup > rhs.materialSetup)  return false;
	if(idBuffer < rhs.idBuffer)  return true;
	if(idBuffer > rhs.idBuffer)  return false;
	if(primitiveTopology < rhs.primitiveTopology)  return true;
	if(primitiveTopology > rhs.primitiveTopology)  return false;
	return projectionHandling < rhs.projectionHandling;
}


ShaderLibrary::GeometryShaderMapKey::GeometryShaderMapKey(const ShaderState& shaderState)
{
	switch(shaderState.primitiveTopology) {
	case vk::PrimitiveTopology::eTriangleList:
	case vk::PrimitiveTopology::eTriangleStrip:
	case vk::PrimitiveTopology::eTriangleFan:
		type = shaderState.idBuffer ? Type::TrianglesIdBuffer : Type::Triangles;
		break;
	case vk::PrimitiveTopology::eLineList:
	case vk::PrimitiveTopology::eLineStrip:
		type = shaderState.idBuffer ? Type::LinesIdBuffer : Type::Lines;
		break;
	default:
		type = Type::Invalid;
	}
}


ShaderLibrary::FragmentShaderMapKey::FragmentShaderMapKey(const ShaderState& shaderState)
{
	switch(shaderState.primitiveTopology) {
	case vk::PrimitiveTopology::eTriangleList:
	case vk::PrimitiveTopology::eTriangleStrip:
	case vk::PrimitiveTopology::eTriangleFan:
		type = shaderState.idBuffer ? Type::TrianglesIdBuffer : Type::Triangles;
		break;
	case vk::PrimitiveTopology::eLineList:
	case vk::PrimitiveTopology::eLineStrip:
		type = shaderState.idBuffer ? Type::LinesIdBuffer : Type::Lines;
		break;
	default:
		type = Type::Invalid;
	}
}
