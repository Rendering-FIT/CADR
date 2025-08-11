/**
 * @file text_block.h
 * @author Christian Saloň
 */

#pragma once

#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <vector>

#include <hb.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "character.h"
#include "font.h"
#include "line_divider.h"
#include "shaper.h"
#include "text_align_strategy.h"
#include "text_segment.h"

namespace vft {

/**
 * @brief Groups together characters which are rendered
 */
class TextBlock {
public:
    std::function<void()> onTextChange; /**< Callback when text in block changes */

protected:
    std::shared_ptr<Font> _font{nullptr}; /**< Current font */
    unsigned int _fontSize{0};            /**< Current Font size */

    unsigned int _maxWidth{0}; /**< Maximum width of text block. 0 indicates unlimited width */
    std::unique_ptr<TextAlignStrategy> _textAlign{nullptr}; /**< Text align of text block */
    double _lineSpacing{1};                                 /**< Line spacing in block */
    glm::vec4 _color{glm::vec4{1.f, 1.f, 1.f, 1.f}};        /**< Color of text in block */
    glm::vec3 _position{glm::vec3{0.f, 0.f, 0.f}};          /**< Position of text block */
    glm::mat4 _transform{glm::mat4(1.f)};                   /**< Transform matrix of text block */

    std::list<TextSegment> _segments{}; /**< Text segments which include characters to render */
    LineDivider _lineDivider{};         /**< Used to divide characters into lines */

public:
    TextBlock();

    void scale(float x, float y, float z);
    void translate(float x, float y, float z);
    void rotate(float x, float y, float z);

    void add(std::u8string text,
             unsigned int start,
             hb_direction_t direction = HB_DIRECTION_LTR,
             hb_script_t script = HB_SCRIPT_LATIN,
             hb_language_t language = hb_language_from_string("en", -1));
    void add(std::u16string text,
             unsigned int start,
             hb_direction_t direction = HB_DIRECTION_LTR,
             hb_script_t script = HB_SCRIPT_LATIN,
             hb_language_t language = hb_language_from_string("en", -1));
    void add(std::u32string text,
             unsigned int start,
             hb_direction_t direction = HB_DIRECTION_LTR,
             hb_script_t script = HB_SCRIPT_LATIN,
             hb_language_t language = hb_language_from_string("en", -1));
    void add(std::u8string text,
             hb_direction_t direction = HB_DIRECTION_LTR,
             hb_script_t script = HB_SCRIPT_LATIN,
             hb_language_t language = hb_language_from_string("en", -1));
    void add(std::u16string text,
             hb_direction_t direction = HB_DIRECTION_LTR,
             hb_script_t script = HB_SCRIPT_LATIN,
             hb_language_t language = hb_language_from_string("en", -1));
    void add(std::u32string text,
             hb_direction_t direction = HB_DIRECTION_LTR,
             hb_script_t script = HB_SCRIPT_LATIN,
             hb_language_t language = hb_language_from_string("en", -1));
    void remove(unsigned int start = std::numeric_limits<unsigned int>::max(), unsigned int count = 1);
    void clear();

    void setFont(std::shared_ptr<Font> font);
    void setFontSize(unsigned int fontSize);
    void setLineSpacing(double lineSpacing);
    void setColor(glm::vec4 color);
    void setPosition(glm::vec3 position);
    void setTransform(glm::mat4 transform);
    void setMaxWidth(unsigned int maxWidth);
    void setTextAlign(std::unique_ptr<TextAlignStrategy> textAlign);

    std::vector<Character> getCharacters();
    unsigned int getCharacterCount();
    std::u32string getUtf32Text();
    unsigned int getCodePointCount();

    std::shared_ptr<Font> getFont() const;
    unsigned int getFontSize() const;
    double getLineSpacing() const;
    glm::vec4 getColor() const;
    glm::vec3 getPosition() const;
    glm::mat4 getTransform() const;
    unsigned int getMaxWidth() const;
    double getWidth() const;
    double getHeight() const;

protected:
    void _updateCharacters();
    void _updateTransform();

    void _updateCharacterPositions(unsigned int start);
    std::list<TextSegment>::iterator _mergeSegmentsIfPossible(std::list<TextSegment>::iterator first,
                                                              std::list<TextSegment>::iterator second);

    TextSegment &_getSegmentBasedOnCodePointGlobalIndex(unsigned int index);
    std::list<TextSegment>::iterator _getSegmentIteratorBasedOnCodePointGlobalIndex(unsigned int index);
    TextSegment &_getSegmentBasedOnCharacterGlobalIndex(unsigned int index);
    std::list<TextSegment>::iterator _getSegmentIteratorBasedOnCharacterGlobalIndex(unsigned int index);
    Character &_getCharacterBasedOnCharacterGlobalIndex(unsigned int index);
    std::vector<Character>::iterator _getCharacterIteratorBasedOnCharacterGlobalIndex(unsigned int index);

    unsigned int _getCodePointGlobalIndexBasedOnSegment(const TextSegment &segment);
    unsigned int _getCharacterGlobalIndexBasedOnSegment(const TextSegment &segment);
    unsigned int _getCharacterGlobalIndexBasedOnCharacter(const Character &character);
};

}  // namespace vft
