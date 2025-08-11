/**
 * @file shaper.h
 * @author Christian Saloň
 */

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <hb-ft.h>
#include <hb.h>

#include "font.h"
#include "unicode.h"

namespace vft {

/**
 * @brief Character data after shaping using harfbuzz
 */
typedef struct {
    uint32_t glyphId; /**< Glyph id of shaped character */
    uint32_t cluster; /**< Cluster id of shaped character (see harfbuzz clusters) */
    double xAdvance;  /**< X advance of shaped character */
    double yAdvance;  /**< Y advance of shaped character */
    double xOffset;   /**< X offset of shaped characer */
    double yOffset;   /**< Y offset of shaped character */
} ShapedCharacter;

/**
 * @brief Shapes text using harfbuzz
 */
class Shaper {
public:
    static std::vector<std::vector<ShapedCharacter>> shape(std::u32string text,
                                                           std::shared_ptr<Font> font,
                                                           unsigned int fontSize,
                                                           hb_direction_t direction = HB_DIRECTION_LTR,
                                                           hb_script_t script = HB_SCRIPT_LATIN,
                                                           hb_language_t language = hb_language_from_string("en", -1));

protected:
    static void _preprocessInput(std::u32string &text);
};

}  // namespace vft
