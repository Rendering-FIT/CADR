/**
 * @file line_divider.h
 * @author Christian Saloň
 */

#pragma once

#include <cstdint>
#include <map>
#include <stdexcept>
#include <vector>

#include "character.h"
#include "unicode.h"

namespace vft {

/**
 * @brief Metrics of one line
 */
typedef struct {
    double width;  /**< Width of line */
    double height; /**< height of line */
    double x;      /**< x coordinate of line start */
    double y;      /**< y coordinate of line start */
} LineData;

/**
 * @brief Divides characters into lines
 */
class LineDivider {
protected:
    double _maxLineSize{0}; /**< Maximum width of lines */
    double _lineSpacing{1}; /**< Line spacing used when calculating positions of lines */

    std::map<unsigned int, LineData>
        _lines{}; /**< Divided lines. Key is index of starting character at given line. Value is line data */
    std::vector<Character> _characters{}; /**< Characters which to divide into lines */

public:
    const std::map<unsigned int, LineData> &divide(unsigned int startCharacterIndex = 0);

    void setCharacters(const std::vector<Character> &characters);
    void setMaxLineSize(double maxLineSize);
    void setLineSpacing(double lineSpacing);

    std::pair<unsigned int, LineData> getLineOfCharacter(unsigned int characterIndex) const;
    const std::map<unsigned int, LineData> &getLines() const;
};

}  // namespace vft
