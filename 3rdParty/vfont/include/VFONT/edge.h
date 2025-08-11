/**
 * @file edge.h
 * @author Christian Saloň
 */

#pragma once

#include <cstdint>

namespace vft {

/*
 * @brief Represents an edge between two vertices
 */
class Edge {
public:
    uint32_t first{0};  /**< First vertex index */
    uint32_t second{0}; /**< Second vertex index */

    Edge(uint32_t first, uint32_t second);
    Edge();

    bool operator==(const Edge &edge) const;
    bool isInverse(const Edge &edge) const;
};

}  // namespace vft
