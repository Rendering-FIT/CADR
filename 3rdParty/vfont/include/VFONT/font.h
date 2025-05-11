/**
 * @file font.h
 * @author Christian Saloň
 */

#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <glm/vec2.hpp>

namespace vft {

/**
 * @brief Represents a freetype font
 */
class Font {
protected:
    FT_Library _ft{nullptr}; /**< Freetype library */
    FT_Face _face{nullptr};  /**< Freetype font face */

    unsigned int _pixelSize{64}; /**< Font size in pixels */

public:
    Font(std::string fontFile);
    Font(uint8_t *buffer, long size);

    void setPixelSize(unsigned int pixelSize);

    glm::vec2 getScalingVector(unsigned int fontSize) const;
    unsigned int getPixelSize() const;
    std::string getFontFamily() const;
    FT_Face getFace() const;
};

}  // namespace vft
