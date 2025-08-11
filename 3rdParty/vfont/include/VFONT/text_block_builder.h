/**
 * @file text_block_builder.h
 * @author Christian Saloň
 */

#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "font.h"
#include "text_align_strategy.h"
#include "text_block.h"

namespace vft {

/**
 * @brief Builder for constructing text blocks
 */
class TextBlockBuilder {
protected:
    TextBlock _block{}; /**< Text block that will be built */

public:
    TextBlockBuilder() = default;

    TextBlockBuilder &setFont(std::shared_ptr<Font> font);
    TextBlockBuilder &setFontSize(unsigned int fontSize);
    TextBlockBuilder &setLineSpacing(double lineSpacing);
    TextBlockBuilder &setMaxWidth(unsigned int width);
    TextBlockBuilder &setColor(glm::vec4 color);
    TextBlockBuilder &setPosition(glm::vec3 position);
    TextBlockBuilder &setTextAlign(std::unique_ptr<TextAlignStrategy> textAlign);

    std::shared_ptr<TextBlock> build();
};

}  // namespace vft
