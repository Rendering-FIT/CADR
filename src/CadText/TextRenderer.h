#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include <VFONT/glyph_cache.h>

#include <CadR/Drawable.h>
#include <CadR/Geometry.h>
#include <CadR/Pipeline.h>
#include <CadR/PrimitiveSet.h>
#include <CadR/Renderer.h>
#include <CadR/StateSet.h>
#include <CadR/VulkanDevice.h>

namespace CadText {

class TextRenderer {
public:
    struct DrawableShaderData {
        glm::vec4 color;
        glm::mat4 model;
    };

protected:
    CadR::Renderer *_renderer{nullptr};
    CadR::StateSet _rootStateSet;
    std::vector<CadR::Pipeline> _pipelines{};

    vk::DeviceAddress _sceneData;

public:
    TextRenderer(CadR::Renderer &renderer);
    ~TextRenderer();

    virtual void update() = 0;

    virtual void createPipelines(vk::RenderPass renderPass, vk::Extent2D extent) = 0;
    void destroyPipelines();

    void setSceneData(vk::DeviceAddress sceneData);

    CadR::StateSet &stateSet();
    std::vector<CadR::Pipeline> &pipelines();
};

}  // namespace CadText
