/**
 * @file glyph_compositor.h
 * @author Christian Saloň
 */

#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec2.hpp>

#include "edge.h"

namespace vft {

/**
 * @brief Composes a triangulated glyph from glyph outlines
 */
class GlyphCompositor {
public:
    static std::vector<uint32_t> triangulate(std::vector<glm::vec2> &vertices, std::vector<Edge> &edges);
};

}  // namespace vft
