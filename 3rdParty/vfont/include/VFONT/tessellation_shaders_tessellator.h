/**
 * @file tessellation_shaders_tessellator.h
 * @author Christian Saloň
 */

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "circular_dll.h"
#include "curve.h"
#include "edge.h"
#include "font.h"
#include "glyph.h"
#include "glyph_cache.h"
#include "glyph_compositor.h"
#include "glyph_mesh.h"
#include "outline.h"
#include "polygon_operator.h"
#include "tessellator.h"

namespace vft {

/**
 * @brief Composes glyphs where the inner triangles are triangulated on the cpu and outer triangles are processed in the
 * tessellation shaders
 */
class TessellationShadersTessellator : public Tessellator {
public:
    /** Index of index buffer containing trinagles in glyph's vector of index buffers */
    static constexpr unsigned int GLYPH_MESH_TRIANGLE_BUFFER_INDEX = 0;
    /** Index of index buffer containing curve segments in glyph's vector of index buffers */
    static constexpr unsigned int GLYPH_MESH_CURVE_BUFFER_INDEX = 1;

protected:
    std::vector<Outline> _firstPolygon{};  /**< Polygon containing processed glyph contours */
    std::vector<Outline> _secondPolygon{}; /**< Polygon containing current contour */

    double area{0}; /**< Signed area of current contour */

public:
    TessellationShadersTessellator();
    ~TessellationShadersTessellator() = default;

    Glyph composeGlyph(uint32_t glyphId, std::shared_ptr<vft::Font> font, unsigned int fontSize = 0) override;

protected:
    bool _isOnLeftSide(const glm::vec2 &lineStartingPoint, const glm::vec2 &lineEndingPoint, const glm::vec2 &point);
};

}  // namespace vft
