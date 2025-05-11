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
#include <VFONT/tessellation_shaders_text_renderer.h>

#include <CadText/TextRenderer.h>

#include <CadR/Drawable.h>
#include <CadR/Geometry.h>
#include <CadR/Pipeline.h>
#include <CadR/PrimitiveSet.h>
#include <CadR/Renderer.h>
#include <CadR/StateSet.h>
#include <CadR/VulkanDevice.h>

namespace CadText {

class TessellationShadersTextRenderer : public virtual vft::TessellationShadersTextRenderer, public TextRenderer {
public:
    static const uint32_t innerVertexShaderSpirv[];
    static const uint32_t innerFragmentShaderSpirv[];
    static const uint32_t outerVertexShaderSpirv[];
    static const uint32_t outerTessellationControlShaderSpirv[];
    static const uint32_t outerTessellationEvaluationShaderSpirv[];
    static const uint32_t outerFragmentShaderSpirv[];

protected:
    std::unordered_map<vft::GlyphKey, unsigned int, vft::GlyphKeyHash> _innerGeometryOffsets{};
    std::vector<CadR::Geometry> _innerGeometries{};
    std::vector<CadR::Drawable> _innerDrawables{};
    std::unordered_map<vft::GlyphKey, unsigned int, vft::GlyphKeyHash> _outerGeometryOffsets{};
    std::vector<CadR::Geometry> _outerGeometries{};
    std::vector<CadR::Drawable> _outerDrawables{};

    vk::ShaderModule _innerVertexShaderModule;
    vk::ShaderModule _innerFragmentShaderModule;
    vk::ShaderModule _outerVertexShaderModule;
    vk::ShaderModule _outerTessellationControlShaderModule;
    vk::ShaderModule _outerTessellationEvaluationShaderModule;
    vk::ShaderModule _outerFragmentShaderModule;

    CadR::StateSet _innerStateSet;
    CadR::StateSet _outerStateSet;

public:
    TessellationShadersTextRenderer(CadR::Renderer &renderer, vk::RenderPass renderPass, vk::Extent2D extent);
    TessellationShadersTextRenderer(CadR::Renderer &renderer);
    ~TessellationShadersTextRenderer();

    void update() override;

    void createPipelines(vk::RenderPass renderPass, vk::Extent2D extent) override;
    void createShaderModules();
};

}  // namespace CadText
