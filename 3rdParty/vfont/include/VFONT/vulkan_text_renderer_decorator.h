/**
 * @file vulkan_text_renderer_decorator.h
 * @author Christian Saloň
 */

#pragma once

#include <memory>
#include <stdexcept>

#include <vulkan/vulkan.h>

#include "font_atlas.h"
#include "i_vulkan_text_renderer.h"
#include "text_renderer.h"
#include "vulkan_text_renderer.h"

namespace vft {

/**
 * @brief Base class for all vulkan text renderer decorators
 */
class VulkanTextRendererDecorator : public IVulkanTextRenderer {
protected:
    VulkanTextRenderer *_renderer; /**< Wrapped vulkan text renderer */

public:
    explicit VulkanTextRendererDecorator(VulkanTextRenderer *renderer);
    virtual ~VulkanTextRendererDecorator() override;

    void add(std::shared_ptr<TextBlock> text) override;
    void draw() override;
    void update() override;

    void setUniformBuffers(UniformBufferObject ubo) override;
    void setViewportSize(unsigned int width, unsigned int height) override;
    void setCache(std::shared_ptr<GlyphCache> cache) override;

    void setCommandBuffer(VkCommandBuffer commandBuffer) override;

    VkPhysicalDevice getPhysicalDevice() override;
    VkDevice getLogicalDevice() override;
    VkCommandPool getCommandPool() override;
    VkQueue getGraphicsQueue() override;
    VkRenderPass getRenderPass() override;
    VkCommandBuffer getCommandBuffer() override;

    void addFontAtlas(const FontAtlas &atlas) override;
};

}  // namespace vft
