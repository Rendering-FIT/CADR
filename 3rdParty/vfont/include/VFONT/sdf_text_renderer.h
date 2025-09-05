/**
 * @file sdf_text_renderer.h
 * @author Christian Saloň
 */

#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include "font_atlas.h"
#include "glyph_cache.h"
#include "sdf_tessellator.h"
#include "text_renderer.h"

namespace vft {

/**
 * @brief Creates vertex and index buffers for all characters in text block to be rendered using signed distance fields
 */
class SdfTextRenderer : public virtual TextRenderer {
public:
    /**
     * @brief Index count and offset of glyph indices in index buffers
     */
    struct GlyphInfo {
        uint32_t boundingBoxOffset; /**< Offset in the glyph's bounding box index buffer */
        uint32_t boundingBoxCount;  /**< Count of the glyph's bounding box indices */
    };

    /**
     * @brief Vertex structure used for rendering text using sdfs
     */
    struct Vertex {
        glm::vec2 position; /**< Vertex position */
        glm::vec2 uv;       /**< Vertex uv */
    };

protected:
    /**
     * Hash map of font atlases containng info about glyphs (key: font family, value: FontTexture object)
     */
    std::unordered_map<std::string, FontAtlas> _fontAtlases{};

    /**
     * Hash map containing glyph info about the index buffers (key: glyph key, value: index count and offsets for each
     * index buffer)
     */
    std::unordered_map<GlyphKey, GlyphInfo, GlyphKeyHash> _offsets{};

    std::vector<Vertex> _vertices{};             /**< Vertex buffer */
    std::vector<uint32_t> _boundingBoxIndices{}; /**< Index buffer containing boundig box indices */

    /** Indicates whether to use alpha blending instead of alpha testing for antialiased edges */
    bool _useSoftEdges{false};
    float _softEdgeMin{0}; /**< Minimum distance threshold used for aplha blending */
    float _softEdgeMax{0}; /**< Maximum distance threshold used for aplha blending */

public:
    SdfTextRenderer(float softEdgeMin, float softEdgeMax);
    SdfTextRenderer();
    virtual ~SdfTextRenderer() = default;

    virtual void update() override;

    virtual void addFontAtlas(const FontAtlas &atlas) override;
};

}  // namespace vft
