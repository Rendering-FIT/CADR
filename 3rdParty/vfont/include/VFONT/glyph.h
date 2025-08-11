/**
 * @file glyph.h
 * @author Christian Saloň
 */

#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <glm/vec2.hpp>

#include "curve.h"
#include "edge.h"
#include "glyph_mesh.h"

namespace vft {

/**
 * @brief Represents glyph data and metrics expressed in font units
 */
class Glyph {
public:
    GlyphMesh mesh{}; /**< Mesh storing vertex and index buffers used for rendering */

protected:
    std::vector<Curve> _curveSegments{}; /**< Curve segments of glyph */
    std::vector<Edge> _lineSegments{};   /**< Line segment of glyph */

    long _width{0};  /**< Width of glyph */
    long _height{0}; /**< Height of glyph */

    long _bearingX{0}; /**< Bearing in the direction of x axis */
    long _bearingY{0}; /**< Bearing in the direction of y axis */

    /**
     * Indicates by how much to increment the X coordinate of pen position.
     * This includes the glyph's width plus the space behind.
     * Used for horizontal layouts
     */
    long _advanceX{0};

    /**
     * Indicates by how much to increment the Y coordinate of pen position.
     * Not specified for horizontal layouts
     */
    long _advanceY{0};

public:
    Glyph() = default;

    void addLineSegment(Edge edge);
    void addCurveSegment(Curve curve);

    void setWidth(long width);
    void setHeight(long height);
    void setBearingX(long bearingX);
    void setBearingY(long bearingY);
    void setAdvanceX(long advanceX);
    void setAdvanceY(long advanceY);

    std::array<glm::vec2, 4> getBoundingBox() const;

    const std::vector<Edge> &getLineSegmentsIndices() const;
    const std::vector<Curve> &getCurveSegmentsIndices() const;
    uint32_t getLineSegmentsIndexCount() const;
    uint32_t getCurveSegmentsIndexCount() const;

    long getWidth() const;
    long getHeight() const;
    long getBearingX() const;
    long getBearingY() const;
    long getAdvanceX() const;
    long getAdvanceY() const;
};

}  // namespace vft
