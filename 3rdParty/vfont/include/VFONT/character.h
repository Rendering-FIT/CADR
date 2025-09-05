/**
 * @file character.h
 * @author Christian Saloň
 */

#pragma once

#include <cstdint>
#include <memory>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "font.h"
#include "glyph.h"

namespace vft {

/**
 * @brief Represents a character which will be rendered
 */
class Character {
protected:
    uint32_t _glyphId{0};   /**< Glyph id used for character in selected font */
    uint32_t _codePoint{0}; /**< Unicode code point of character */

    std::shared_ptr<Font> _font{nullptr}; /**< Font of character */
    unsigned int _fontSize{0};            /**< Font size of character */

    glm::vec2 _advance{0.f, 0.f}; /**< Value added to pen after drawing character expressed in pixels */
    glm::vec2 _offset{0.f, 0.f};  /**< Value added to pen before drawing character expressed in pixels */

    glm::vec2 _position{0.f, 0.f}; /**< Character position, coordinates represent the bottom left corner of character */
    glm::mat4 _modelMatrix{1.f};   /**< Character model matrix */
    glm::mat4 _parentMatrix{1.f};  /**< Model matrix of text block */

public:
    Character(uint32_t glyphId, uint32_t codePoint, std::shared_ptr<Font> font, unsigned int fontSize);

    void setAdvance(glm::vec2 advance);
    void setOffset(glm::vec2 offset);
    void setPosition(glm::vec2 position);
    void setTransform(glm::mat4 transform);

    uint32_t getGlyphId() const;
    uint32_t getCodePoint() const;
    glm::vec2 getAdvance() const;
    glm::vec2 getOffset() const;
    glm::vec2 getPosition() const;
    glm::mat4 getModelMatrix() const;
    std::shared_ptr<Font> getFont() const;
    unsigned int getFontSize() const;

protected:
    void _updateModelMatrix();
};

}  // namespace vft
