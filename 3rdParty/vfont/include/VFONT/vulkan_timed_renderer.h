/**
 * @file vulkan_timed_renderer.h
 * @author Christian Saloň
 */

#pragma once

#include <stdexcept>
#include <vector>

#include <vulkan/vulkan.h>

#include "vulkan_text_renderer.h"
#include "vulkan_text_renderer_decorator.h"

namespace vft {

/**
 * @brief Vulkan text renderer that tracks draw time using timestamps
 */
class VulkanTimedRenderer : public VulkanTextRendererDecorator {
protected:
    double _timestampPeriod{1e-9}; /**< Vulkan timestamp period */

    VkQueryPool _queryPool{nullptr}; /**< Vulkan query pool to read timestamps from */

public:
    VulkanTimedRenderer(VulkanTextRenderer *renderer);
    ~VulkanTimedRenderer() override;

    void draw() override;

    double readTimestamps();
    void resetQueryPool();

protected:
    double _getTimestampPeriod();
};

}  // namespace vft
