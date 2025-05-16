/**
 * @file text_align_strategy.h
 * @author Christian Saloň
 */

#pragma once

#include <stdexcept>

#include <glm/vec2.hpp>

namespace vft {

/**
 * @brief Base class for all text aligns of text block
 */
class TextAlignStrategy {
public:
    virtual glm::vec2 getLineOffset(double lineSize, double maxLineSize) const = 0;
};

/**
 * @brief Left text align of text block
 */
class LeftTextAlign : public TextAlignStrategy {
public:
    glm::vec2 getLineOffset(double lineSize, double maxLineSize) const override;
};

/**
 * @brief Center text align of text block
 */
class CenterTextAlign : public TextAlignStrategy {
public:
    glm::vec2 getLineOffset(double lineSize, double maxLineSize) const override;
};

/**
 * @brief Right text align of text block
 */
class RightTextAlign : public TextAlignStrategy {
public:
    glm::vec2 getLineOffset(double lineSize, double maxLineSize) const override;
};

}  // namespace vft
