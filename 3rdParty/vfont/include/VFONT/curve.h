/**
 * @file curve.h
 * @author Christian Saloň
 */

#pragma once

#include <cstdint>

namespace vft {

/**
 * @brief Represents a quadratic bezier curve
 */
class Curve {
public:
    uint32_t start{0};   /**< Start point vertex index */
    uint32_t control{0}; /**< Control point vertex index */
    uint32_t end{0};     /**< End point vertex index */

    Curve(uint32_t start, uint32_t control, uint32_t end);
    Curve();

    bool operator==(const Curve &curve) const;
};

}  // namespace vft
