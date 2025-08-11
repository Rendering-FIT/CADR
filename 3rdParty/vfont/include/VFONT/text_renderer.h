/**
 * @file text_renderer.h
 * @author Christian Saloň
 */

#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

#include <glm/mat4x4.hpp>

#include "font_atlas.h"
#include "glyph_cache.h"
#include "tessellator.h"
#include "text_block.h"

namespace vft {

/**
 * @brief Lists all available algorithms for rendering text
 */
enum class TessellationStrategy { SDF, TRIANGULATION, TESSELLATION_SHADERS, WINDING_NUMBER };

/**
 * @brief Uniform buffer object
 */
class UniformBufferObject {
public:
    glm::mat4 view;       /**< View matrix */
    glm::mat4 projection; /**< Projection matrix */

    UniformBufferObject(glm::mat4 view, glm::mat4 projection) : view{view}, projection{projection} {}
    UniformBufferObject() : view{glm::mat4(1.f)}, projection{glm::mat4(1.f)} {}
};

/**
 * @brief Base class for text renderers
 */
class TextRenderer {
protected:
    UniformBufferObject _ubo{glm::mat4{1.f}, glm::mat4{1.f}}; /**< Unifomr buffer object */
    unsigned int _viewportWidth{0};                           /**< Viewport width */
    unsigned int _viewportHeight{0};                          /**< Viewport height */

    std::vector<std::shared_ptr<TextBlock>> _textBlocks{}; /**< All text blocks to be rendered */
    std::shared_ptr<GlyphCache> _cache{nullptr}; /**< Glyph cache stores glyphs and reads them before rendering */
    std::unique_ptr<Tessellator> _tessellator{nullptr}; /**< Tessellator to compose glyphs for rendering */

public:
    TextRenderer();
    virtual ~TextRenderer() = default;

    virtual void add(std::shared_ptr<TextBlock> text);
    virtual void update() = 0;

    virtual void addFontAtlas(const FontAtlas &atlas);

    virtual void setUniformBuffers(UniformBufferObject ubo);
    virtual void setViewportSize(unsigned int width, unsigned int height);
    virtual void setCache(std::shared_ptr<GlyphCache> cache);
};

}  // namespace vft
