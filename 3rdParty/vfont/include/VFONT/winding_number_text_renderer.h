/**
 * @file winding_number_text_renderer.h
 * @author Christian Saloň
 */

#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include "glyph_cache.h"
#include "text_renderer.h"
#include "winding_number_tessellator.h"

namespace vft {

/**
 * @brief Basic implementation of vulkan text renderer where glyphs are filled using the winding number algorithm in the
 * fragment shader
 */

/**
 * @brief Creates vertex, index buffer and ssbo for all characters in text block which are rendered using the winding
 * number algorithm
 */
class WindingNumberTextRenderer : public virtual TextRenderer {
public:
    /**
     * @brief Index count and offset of glyph indices in index buffers
     */
    struct GlyphInfo {
        uint32_t boundingBoxOffset;  /**< Offset in the glyph's bounding box index buffer */
        uint32_t boundingBoxCount;   /**< Count of the glyph's bounding box indices */
        uint32_t segmentsInfoOffset; /**< Offset in the glyph's segments info buffer */
    };

    /**
     * @brief Represents a glyph's information about line and curve segments stored in ssbo
     */
    struct SegmentsInfo {
        uint32_t lineSegmentsStartIndex;  /**< Index into the ssbo where glyph's line segments start */
        uint32_t lineSegmentsCount;       /**< Number of glyph's line segments */
        uint32_t curveSegmentsStartIndex; /**< Index into the ssbo where glyph's curve segments start */
        uint32_t curveSegmentsCount;      /**< Number of glyph's curve segments */
    };

    /**
     * @brief Represents a line segment
     */
    struct LineSegment {
        glm::vec2 start; /**< Start of line segment */
        glm::vec2 end;   /**< End of line segment */
    };

    /**
     * @brief Represents a quadratic bezier curve segment
     */
    struct CurveSegment {
        glm::vec2 start;   /**< Start of curve segment */
        glm::vec2 control; /**< Control point of curve segment */
        glm::vec2 end;     /**< End of curve segment */
    };

protected:
    /**
     * Hash map containing glyph info about the index buffers (key: glyph key, value: index count and offsets for each
     * index buffer)
     */
    std::unordered_map<GlyphKey, GlyphInfo, GlyphKeyHash> _offsets{};

    std::vector<glm::vec2> _vertices{};          /**< Vertex buffer */
    std::vector<uint32_t> _boundingBoxIndices{}; /**< Index buffer containing boundig box indices */
    std::vector<glm::vec2> _segments{};          /**< Contains line and curve segments of all glpyhs */
    std::vector<SegmentsInfo> _segmentsInfo{};   /**< Contains segment info of all glpyhs */

public:
    WindingNumberTextRenderer();
    virtual ~WindingNumberTextRenderer() = default;

    virtual void update() override;
};

}  // namespace vft
