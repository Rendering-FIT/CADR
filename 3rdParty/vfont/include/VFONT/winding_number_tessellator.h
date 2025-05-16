/**
 * @file winding_number_tessellator.h
 * @author Christian Saloň
 */

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "curve.h"
#include "edge.h"
#include "font.h"
#include "glyph.h"
#include "glyph_cache.h"
#include "glyph_compositor.h"
#include "glyph_mesh.h"
#include "tessellator.h"

namespace vft {

/**
 * @brief Composes glyphs that are rendered using the winding number algorithm
 */
class WindingNumberTessellator : public Tessellator {
public:
    /** Index of index buffer containing the bounding box in glyph's vector of index buffers */
    static constexpr unsigned int GLYPH_MESH_BOUNDING_BOX_BUFFER_INDEX = 0;
    /** Index of index buffer containing curve segments in glyph's vector of index buffers */
    static constexpr unsigned int GLYPH_MESH_CURVE_BUFFER_INDEX = 1;
    /** Index of index buffer containing line segments in glyph's vector of index buffers */
    static constexpr unsigned int GLYPH_MESH_LINE_BUFFER_INDEX = 2;

    WindingNumberTessellator();
    ~WindingNumberTessellator() = default;

    Glyph composeGlyph(uint32_t glyphId, std::shared_ptr<vft::Font> font, unsigned int fontSize = 0) override;
};

}  // namespace vft
