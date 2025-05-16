/**
 * @file text_segment.h
 * @author Christian Saloň
 */

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <hb.h>
#include <glm/mat4x4.hpp>

#include "character.h"
#include "font.h"
#include "shaper.h"

namespace vft {

/**
 * @brief Groups together characters which have same properties (font and font size)
 */
class TextSegment {
protected:
    std::shared_ptr<Font> _font{nullptr}; /**< Font used by characters in segment */
    unsigned int _fontSize{0};            /**< Font size used in segment */

    /** Direction in which to render text(e.g., left - to - right, right - to - left) */
    hb_direction_t _direction{HB_DIRECTION_LTR};
    hb_script_t _script{HB_SCRIPT_LATIN};                       /**< Script of text in segment */
    hb_language_t _language{hb_language_from_string("en", -1)}; /**< Language of text in segment */

    glm::mat4 _transform{1.f}; /**< Transform matrix of text block */

    std::u32string _text{};               /**< Utf-32 text to render */
    std::vector<Character> _characters{}; /**< Characters to render */

public:
    TextSegment(std::shared_ptr<Font> font,
                unsigned int fontSize,
                hb_direction_t direction = HB_DIRECTION_LTR,
                hb_script_t script = HB_SCRIPT_LATIN,
                hb_language_t language = hb_language_from_string("en", -1));

    void add(const std::u32string &text, unsigned int start = std::numeric_limits<unsigned int>::max());
    void remove(unsigned int start, unsigned int count = 1);

    void setTransform(glm::mat4 transform);
    glm::mat4 getTransform() const;

    const std::u32string &getText();
    std::vector<Character> &getCharacters();
    unsigned int getCodePointCount() const;
    unsigned int getCharacterCount() const;

    std::shared_ptr<Font> getFont() const;
    unsigned int getFontSize() const;
    hb_direction_t getDirection() const;
    hb_script_t getScript() const;
    hb_language_t getLanguage() const;

protected:
    void _shape();
};

}  // namespace vft
