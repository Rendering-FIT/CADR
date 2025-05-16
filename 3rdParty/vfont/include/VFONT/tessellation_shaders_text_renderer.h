/**
 * @file tessellation_shaders_text_renderer.h
 * @author Christian Saloň
 */

#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "glyph_cache.h"
#include "tessellation_shaders_tessellator.h"
#include "text_renderer.h"

namespace vft {

/**
 * @brief Creates vertex and index buffers for all characters in text block to be rendered using tessellation shaders
 */
class TessellationShadersTextRenderer : public virtual TextRenderer {
public:
    /**
     * @brief Index count and offset of glyph indices in index buffers
     */
    struct GlyphInfo {
        uint32_t lineSegmentsOffset;  /**< Offset in the glyph's line segments index buffer */
        uint32_t lineSegmentsCount;   /**< Count of the glyph's line segments indices */
        uint32_t curveSegmentsOffset; /**< Offset in the glyph's curve segments index buffer */
        uint32_t curveSegmentsCount;  /**< Count of the glyph's curve segments indices */
    };

protected:
    /**
     * Hash map containing glyph info about the index buffers (key: glyph key, value: index count and offsets for each
     * index buffer)
     */
    std::unordered_map<GlyphKey, GlyphInfo, GlyphKeyHash> _offsets{};

    std::vector<glm::vec2> _vertices{};            /**< Vertex buffer */
    std::vector<uint32_t> _lineSegmentsIndices{};  /**< Index buffer */
    std::vector<uint32_t> _curveSegmentsIndices{}; /**< Index buffer */

public:
    TessellationShadersTextRenderer();
    virtual ~TessellationShadersTextRenderer() = default;

    virtual void update() override;
};

}  // namespace vft
