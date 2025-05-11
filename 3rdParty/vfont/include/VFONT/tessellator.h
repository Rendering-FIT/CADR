/**
 * @file tessellator.h
 * @author Christian Saloň
 */

#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include <glm/glm.hpp>

#include "curve.h"
#include "edge.h"
#include "font.h"
#include "glyph.h"

namespace vft {

/**
 * @brief Base class for all tessellators. Composes a glyph based on rendering method
 */
class Tessellator {
protected:
    FT_Outline_MoveToFunc _moveToFunc{nullptr};   /**< Freetype moveTo function */
    FT_Outline_LineToFunc _lineToFunc{nullptr};   /**< Freetype lineTo function */
    FT_Outline_ConicToFunc _conicToFunc{nullptr}; /**< Freetype conicTo function */
    FT_Outline_CubicToFunc _cubicToFunc{nullptr}; /**< Freetype cubicTo function */

    uint32_t vertexIndex{0};             /**< Index of next unique vertex */
    uint32_t contourStartVertexIndex{0}; /**< Index of vertex that starts current contour */
    glm::vec2 lastVertex{0, 0};          /**< Last processed vertex */
    uint32_t lastVertexIndex{0};         /**< Index of last processed vertex */
    unsigned int contourCount{0};        /**< Number of processed contours */

    Glyph _currentGlyph{}; /**< Glyph that is currently being composed */

public:
    Tessellator();
    ~Tessellator() = default;

    virtual Glyph composeGlyph(uint32_t glyphId, std::shared_ptr<vft::Font> font, unsigned int fontSize = 0) = 0;

protected:
    Glyph _composeGlyph(uint32_t glyphId, std::shared_ptr<vft::Font> font);
    uint32_t _getVertexIndex(const glm::vec2 &vertex);
};

}  // namespace vft
