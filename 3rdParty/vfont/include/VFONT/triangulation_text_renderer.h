/**
 * @file triangulation_text_renderer.h
 * @author Christian Saloň
 */

#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <glm/vec2.hpp>

#include "glyph_cache.h"
#include "text_renderer.h"
#include "triangulation_tessellator.h"

namespace vft {

/**
 * @brief Creates vertex and index buffers for all characters in text block tessellated and triangulized on the cpu
 */
class TriangulationTextRenderer : public virtual TextRenderer {
public:
    /** Index into the array containing index buffer offsets of glyph's triangles */
    static constexpr unsigned int TRIANGLE_OFFSET_BUFFER_INDEX = 0;

    /**
     * @brief Index count and offset of glyph indices in index buffers
     */
    struct GlyphInfo {
        uint32_t indicesOffset; /**< Offset in the glyph's triangle index buffer */
        uint32_t indicesCount;  /**< Count of the glyph's triangle indices */
    };

protected:
    /**
     * Hash map containing glyph info about the index buffers (key: glyph key, value: index count and offsets for each
     * index buffer)
     */
    std::unordered_map<GlyphKey, GlyphInfo, GlyphKeyHash> _offsets{};

    std::vector<glm::vec2> _vertices{}; /**< Vertex buffer */
    std::vector<uint32_t> _indices{};   /**< Index buffer */

public:
    TriangulationTextRenderer();
    virtual ~TriangulationTextRenderer() = default;

    virtual void update() override;
};

}  // namespace vft
