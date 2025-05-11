#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include <VFONT/glyph_cache.h>
#include <VFONT/triangulation_text_renderer.h>

#include <CadText/TextRenderer.h>

#include <CadR/Drawable.h>
#include <CadR/Geometry.h>
#include <CadR/Pipeline.h>
#include <CadR/PrimitiveSet.h>
#include <CadR/Renderer.h>
#include <CadR/StateSet.h>
#include <CadR/VulkanDevice.h>

namespace CadText {

class TriangulationTextRenderer : public virtual vft::TriangulationTextRenderer, public TextRenderer {
public:
    static const uint32_t vertexShaderSpirv[];
    static const uint32_t fragmentShaderSpirv[];

protected:
    std::unordered_map<vft::GlyphKey, unsigned int, vft::GlyphKeyHash> _geometryOffsets{};
    std::vector<CadR::Geometry> _geometries{};
    std::vector<CadR::Drawable> _drawables{};

    vk::ShaderModule _vertexShaderModule;
    vk::ShaderModule _fragmentShaderModule;

    CadR::StateSet _triangleStateSet;

public:
    TriangulationTextRenderer(CadR::Renderer &renderer, vk::RenderPass renderPass, vk::Extent2D extent);
    TriangulationTextRenderer(CadR::Renderer &renderer);
    ~TriangulationTextRenderer();

    void update() override;

    void createPipelines(vk::RenderPass renderPass, vk::Extent2D extent) override;
    void createShaderModules();
};

}  // namespace CadText
