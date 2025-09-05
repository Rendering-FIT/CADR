/**
 * @file outline.h
 * @author Christian Saloň
 */

#pragma once

#include "circular_dll.h"
#include "edge.h"

namespace vft {

/**
 * @brief Outline of polygom
 */
class Outline {
public:
    /**
     * @brief Orientation of outline
     */
    enum class Orientation {
        CW, /**< Clockwise */
        CCW /**< Counter clockwise */
    };

public:
    CircularDLL<Edge> edges{};                /**< Egdes of outline */
    Orientation orientation{Orientation::CW}; /**< Orientation of outline */

public:
    Outline() = default;
    ~Outline() = default;
};

}  // namespace vft
