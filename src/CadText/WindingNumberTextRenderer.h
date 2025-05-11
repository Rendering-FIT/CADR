#pragma once

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include <VFONT/glyph_cache.h>
#include <VFONT/winding_number_text_renderer.h>

#include <CadText/TextRenderer.h>

#include <CadR/Drawable.h>
#include <CadR/Geometry.h>
#include <CadR/Pipeline.h>
#include <CadR/PrimitiveSet.h>
#include <CadR/Renderer.h>
#include <CadR/StateSet.h>
#include <CadR/VulkanDevice.h>

namespace CadText {

class WindingNumberTextRenderer : public virtual vft::WindingNumberTextRenderer, public TextRenderer {
public:
    struct DrawableShaderData {
        glm::vec4 color;
        glm::mat4 model;
        unsigned int lineSegmentCount;
        unsigned int curveSegmentCount;
        glm::vec2 padding1;
        glm::vec2 segments[];
    };

    static const uint32_t vertexShaderSpirv[];
    static const uint32_t fragmentShaderSpirv[];

protected:
    std::unordered_map<vft::GlyphKey, unsigned int, vft::GlyphKeyHash> _geometryOffsets{};
    std::vector<CadR::Geometry> _geometries{};
    std::vector<CadR::Drawable> _drawables{};

    vk::ShaderModule _vertexShaderModule;
    vk::ShaderModule _fragmentShaderModule;

    CadR::StateSet _boundingBoxStateSet;

public:
    WindingNumberTextRenderer(CadR::Renderer &renderer, vk::RenderPass renderPass, vk::Extent2D extent);
    WindingNumberTextRenderer(CadR::Renderer &renderer);
    ~WindingNumberTextRenderer();

    void update() override;

    void createPipelines(vk::RenderPass renderPass, vk::Extent2D extent) override;
    void createShaderModules();
};

}  // namespace CadText
