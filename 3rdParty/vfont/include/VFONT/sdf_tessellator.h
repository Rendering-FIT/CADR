/**
 * @file sdf_tessellator.h
 * @author Christian Saloň
 */

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "font.h"
#include "glyph.h"
#include "glyph_mesh.h"
#include "tessellator.h"

namespace vft {

/**
 * @brief Composes glyphs that are rendered using signed distance fields
 */
class SdfTessellator : public Tessellator {
public:
    /** Index of index buffer containing the bounding box in glyph's vector of index buffers */
    static constexpr unsigned int GLYPH_MESH_BOUNDING_BOX_BUFFER_INDEX = 0;

    SdfTessellator();
    ~SdfTessellator() = default;

    Glyph composeGlyph(uint32_t glyphId, std::shared_ptr<vft::Font> font, unsigned int fontSize = 0) override;
};

}  // namespace vft
