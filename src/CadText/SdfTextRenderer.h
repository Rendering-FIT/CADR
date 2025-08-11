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
#include <VFONT/sdf_text_renderer.h>

#include <CadText/TextRenderer.h>

#include <CadR/Drawable.h>
#include <CadR/Geometry.h>
#include <CadR/ImageAllocation.h>
#include <CadR/Pipeline.h>
#include <CadR/PrimitiveSet.h>
#include <CadR/Renderer.h>
#include <CadR/Sampler.h>
#include <CadR/StateSet.h>
#include <CadR/Texture.h>
#include <CadR/VulkanDevice.h>

namespace CadText {

class SdfTextRenderer : public virtual vft::SdfTextRenderer, public TextRenderer {
public:
    static const uint32_t vertexShaderSpirv[];
    static const uint32_t fragmentShaderSpirv[];

protected:
    std::unordered_map<vft::GlyphKey, unsigned int, vft::GlyphKeyHash> _geometryOffsets{};
    std::vector<CadR::Geometry> _geometries{};
    std::vector<CadR::Drawable> _drawables{};

    std::unordered_map<std::string, unsigned int> _textureOffsets{};
    std::vector<CadR::ImageAllocation> _images{};
    std::vector<CadR::Texture> _textures{};
    std::vector<std::unique_ptr<CadR::StateSet>> _textureStateSets{};
    std::vector<vk::DescriptorSetLayout> _descriptorSetLayouts{};

    vk::ShaderModule _vertexShaderModule;
    vk::ShaderModule _fragmentShaderModule;

    CadR::StateSet _boundingBoxStateSet;
    CadR::Sampler _sampler;

public:
    SdfTextRenderer(CadR::Renderer &renderer, vk::RenderPass renderPass, vk::Extent2D extent);
    SdfTextRenderer(CadR::Renderer &renderer);
    ~SdfTextRenderer();

    void update() override;
    void addFontAtlas(const vft::FontAtlas &atlas);

    void createPipelines(vk::RenderPass renderPass, vk::Extent2D extent) override;
    void createShaderModules();
};

}  // namespace CadText
