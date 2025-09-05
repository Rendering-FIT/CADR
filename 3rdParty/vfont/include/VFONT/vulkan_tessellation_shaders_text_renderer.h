/**
 * @file vulkan_tessellation_shaders_text_renderer.h
 * @author Christian Saloň
 */

#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

#include "glyph_cache.h"
#include "tessellation_shaders_tessellator.h"
#include "tessellation_shaders_text_renderer.h"
#include "vulkan_text_renderer.h"

namespace vft {

/**
 * @brief Basic implementation of vulkan text renderer where outer triangles are tessellated using shaders and inner
 * triangles are triangulized on the cpu
 */
class VulkanTessellationShadersTextRenderer : public VulkanTextRenderer, public TessellationShadersTextRenderer {
public:
    /**
     * @brief Vulkan push constants
     */
    struct CharacterPushConstants {
        glm::mat4 model;         /**< Model matrix of character */
        glm::vec4 color;         /**< Color of character */
        uint32_t viewportWidth;  /**< Viewport width */
        uint32_t viewportHeight; /**< Viewport height */
    };

protected:
    VkBuffer _vertexBuffer{nullptr};             /**< Vulkan vertex buffer */
    VkDeviceMemory _vertexBufferMemory{nullptr}; /**< Vulkan vertex buffer memory */
    VkBuffer _lineSegmentsIndexBuffer{nullptr};  /**< Vulkan index buffer for line segments forming inner triangles */
    VkDeviceMemory _lineSegmentsIndexBufferMemory{
        nullptr}; /**< Vulkan index buffer memory for line segments forming inner triangles */
    VkBuffer _curveSegmentsIndexBuffer{nullptr};             /**< Vulkan index buffer for curve segments */
    VkDeviceMemory _curveSegmentsIndexBufferMemory{nullptr}; /**< Vulkan index buffer memory for curve segments */

    VkPipelineLayout _lineSegmentsPipelineLayout{nullptr};  /**< Vulkan pipeline layout for glpyh's triangles */
    VkPipeline _lineSegmentsPipeline{nullptr};              /**< Vulkan pipeline for glpyh's triangles */
    VkPipelineLayout _curveSegmentsPipelineLayout{nullptr}; /**< Vulkan pipeline layout for glpyh's curve segments */
    VkPipeline _curveSegmentsPipeline{nullptr};             /**< Vulkan pipeline for glpyh's curve segments */

public:
    VulkanTessellationShadersTextRenderer(VkPhysicalDevice physicalDevice,
                                          VkDevice logicalDevice,
                                          VkQueue graphicsQueue,
                                          VkCommandPool commandPool,
                                          VkRenderPass renderPass,
                                          VkSampleCountFlagBits msaaSampleCount = VK_SAMPLE_COUNT_1_BIT,
                                          VkCommandBuffer commandBuffer = nullptr);
    virtual ~VulkanTessellationShadersTextRenderer();

    void draw() override;
    void update() override;

protected:
    void _createLineSegmentsPipeline();
    void _createCurveSegmentsPipeline();
};

}  // namespace vft
