#pragma once

#include <vulkan/vulkan.hpp>

namespace CadR {
class VulkanDevice;
}

namespace CadPL {

struct ShaderState;



class CADPL_EXPORT ShaderGenerator {
public:
	static [[nodiscard]] vk::ShaderModule createVertexShader(const ShaderState& state, CadR::VulkanDevice& device);
	static [[nodiscard]] vk::ShaderModule createGeometryShader(const ShaderState& state, CadR::VulkanDevice& device);
	static [[nodiscard]] vk::ShaderModule createFragmentShader(const ShaderState& state, CadR::VulkanDevice& device);
	static vk::UniqueHandle<vk::ShaderModule, CadR::VulkanDevice> createVertexShaderUnique(const ShaderState& state, CadR::VulkanDevice& device);
	static vk::UniqueHandle<vk::ShaderModule, CadR::VulkanDevice> createGeometryShaderUnique(const ShaderState& state, CadR::VulkanDevice& device);
	static vk::UniqueHandle<vk::ShaderModule, CadR::VulkanDevice> createFragmentShaderUnique(const ShaderState& state, CadR::VulkanDevice& device);
};


// inline functions
inline vk::UniqueHandle<vk::ShaderModule, CadR::VulkanDevice> ShaderGenerator::createVertexShaderUnique(const ShaderState& state, CadR::VulkanDevice& device)  { return vk::UniqueHandle<vk::ShaderModule, CadR::VulkanDevice>(createVertexShader(state, device)); }
inline vk::UniqueHandle<vk::ShaderModule, CadR::VulkanDevice> ShaderGenerator::createGeometryShaderUnique(const ShaderState& state, CadR::VulkanDevice& device)  { return vk::UniqueHandle<vk::ShaderModule, CadR::VulkanDevice>(createGeometryShader(state, device)); }
inline vk::UniqueHandle<vk::ShaderModule, CadR::VulkanDevice> ShaderGenerator::createFragmentShaderUnique(const ShaderState& state, CadR::VulkanDevice& device)  { return vk::UniqueHandle<vk::ShaderModule, CadR::VulkanDevice>(createFragmentShader(state, device)); }


}
