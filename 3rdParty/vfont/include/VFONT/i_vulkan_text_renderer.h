/**
 * @file i_vulkan_text_renderer.h
 * @author Christian Saloň
 */

#pragma once

#include <vulkan/vulkan.h>

#include "text_renderer.h"

namespace vft {

/**
 * @brief Interface for Vulkan text renderers
 */
class IVulkanTextRenderer : public virtual TextRenderer {
public:
    virtual ~IVulkanTextRenderer() = default;

    virtual void draw() = 0;

    // Setters for vulkan objects
    virtual void setCommandBuffer(VkCommandBuffer commandBuffer) = 0;

    // Getters for vulkan objects
    virtual VkPhysicalDevice getPhysicalDevice() = 0;
    virtual VkDevice getLogicalDevice() = 0;
    virtual VkCommandPool getCommandPool() = 0;
    virtual VkQueue getGraphicsQueue() = 0;
    virtual VkRenderPass getRenderPass() = 0;
    virtual VkCommandBuffer getCommandBuffer() = 0;
};

}  // namespace vft
