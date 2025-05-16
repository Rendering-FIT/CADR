/**
 * @file vulkan_text_renderer.h
 * @author Christian Saloň
 */

#pragma once

#include <fstream>
#include <memory>
#include <stdexcept>

#include <vulkan/vulkan.h>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include "i_vulkan_text_renderer.h"

namespace vft {

/**
 * @brief Push constants used by vulkan for rendering characters
 */
class CharacterPushConstants {
public:
    glm::mat4 model; /**< Model matrix of character */
    glm::vec4 color; /**< Color of character */

    CharacterPushConstants(glm::mat4 model, glm::vec4 color) : model{model}, color{color} {};
    CharacterPushConstants() : model{glm::mat4{1}}, color{glm::vec4{1}} {};
};

/**
 * @brief Base class for basic implementations of vulkan text renderers
 */
class VulkanTextRenderer : public IVulkanTextRenderer {
protected:
    VkSampleCountFlagBits _msaaSampleCount{VK_SAMPLE_COUNT_1_BIT};

    VkPhysicalDevice _physicalDevice{nullptr}; /**< Vulkan physical device */
    VkDevice _logicalDevice{nullptr};          /**< Vulkan logical device */
    VkQueue _graphicsQueue{nullptr};           /**< Vulkan graphics queue */
    VkCommandPool _commandPool{nullptr};       /**< Vulkan command pool */
    VkRenderPass _renderPass{nullptr};         /**< Vulkan render pass */
    VkCommandBuffer _commandBuffer{nullptr};   /**< Vulkan command buffer used in draw calls */

    VkDescriptorPool _descriptorPool{nullptr}; /**< Vulkan descriptor pool */

    VkDescriptorSetLayout _uboDescriptorSetLayout{nullptr}; /**< Uniform buffer descriptor set layout */
    VkDescriptorSet _uboDescriptorSet{nullptr};             /**< Uniform buffer descriptor set */

    VkBuffer _uboBuffer{nullptr};       /**< Vulkan buffer for the uniform buffer object */
    VkDeviceMemory _uboMemory{nullptr}; /**< Vulkan memory for the uniform buffer object */
    void *_mappedUbo{nullptr};          /**< Pointer to the mapped memory for the uniform buffer object */

public:
    VulkanTextRenderer(VkPhysicalDevice physicalDevice,
                       VkDevice logicalDevice,
                       VkQueue graphicsQueue,
                       VkCommandPool commandPool,
                       VkRenderPass renderPass,
                       VkSampleCountFlagBits msaaSampleCount = VK_SAMPLE_COUNT_1_BIT,
                       VkCommandBuffer commandBuffer = nullptr);
    virtual ~VulkanTextRenderer();

    void setUniformBuffers(UniformBufferObject ubo) override;
    void setCommandBuffer(VkCommandBuffer commandBuffer) override;

    VkPhysicalDevice getPhysicalDevice() override;
    VkDevice getLogicalDevice() override;
    VkCommandPool getCommandPool() override;
    VkQueue getGraphicsQueue() override;
    VkRenderPass getRenderPass() override;
    VkCommandBuffer getCommandBuffer() override;

protected:
    void _initialize();

    virtual void _createDescriptorPool();
    void _createUbo();
    void _createUboDescriptorSetLayout();
    void _createUboDescriptorSet();

    std::vector<char> _readFile(std::string fileName);
    VkShaderModule _createShaderModule(const std::vector<char> &shaderCode);

    void _stageAndCreateVulkanBuffer(void *data,
                                     VkDeviceSize size,
                                     VkBufferUsageFlags destinationUsage,
                                     VkBuffer &destinationBuffer,
                                     VkDeviceMemory &destinationMemory);
    void _createBuffer(VkDeviceSize size,
                       VkBufferUsageFlags usage,
                       VkMemoryPropertyFlags properties,
                       VkBuffer &buffer,
                       VkDeviceMemory &bufferMemory);
    void _copyBuffer(VkBuffer sourceBuffer, VkBuffer destinationBuffer, VkDeviceSize size);
    void _destroyBuffer(VkBuffer &buffer, VkDeviceMemory &bufferMemory);
    uint32_t _selectMemoryType(uint32_t memoryType, VkMemoryPropertyFlags properties);
    VkCommandBuffer _beginOneTimeCommands();
    void _endOneTimeCommands(VkCommandBuffer commandBuffer);
};

}  // namespace vft
