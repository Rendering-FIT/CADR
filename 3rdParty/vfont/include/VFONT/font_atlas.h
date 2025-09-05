/**
 * @file font_atlas.h
 * @author Christian Saloň
 */

#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <unordered_map>

#include <glm/vec2.hpp>

#include "font.h"
#include "glyph.h"
#include "glyph_mesh.h"

namespace vft {

/**
 * @brief Font atlas containing sdfs of selected glyphs
 */
class FontAtlas {
public:
    /**
     * @brief Range of unicode characters
     */
    class CharacterRange {
    public:
        char32_t start; /**< First unicode code point of range */
        char32_t end;   /**< Last unicode code point of range */

        CharacterRange(char32_t start, char32_t end) : start{start}, end{end} {}

        unsigned int size() const { return end - start + 1; }
    };

    /**
     * @brief UVs of glyph in font atlas
     */
    struct GlyphInfo {
        glm::vec2 uvTopLeft;     /**< UV of top left corner of glyph */
        glm::vec2 uvBottomRight; /**< UV of bottom right corner of glyph */
    };

    struct BitmapInfo {
        uint32_t glyphId;
        unsigned int width;
        unsigned int height;

        bool operator==(const BitmapInfo &other) const {
            return this->glyphId == other.glyphId && this->width == other.width && this->height == other.height;
        }
    };

    struct BitmapInfoHash {
        std::size_t operator()(const BitmapInfo &b) const {
            return std::hash<uint32_t>()(b.glyphId) ^ (std::hash<unsigned int>()(b.width) << 1) ^
                   (std::hash<unsigned int>()(b.height) << 2);
        }
    };

protected:
    std::string _fontFamily{};  /**< Font family of font atlas */
    unsigned int _width{1024};  /**< Width of font atlas */
    unsigned int _height{1024}; /**< Height of font atlas */

    std::vector<uint8_t> _texture{}; /**< Texture containing sdfs of selected glyphs */
    /**
     * Hash map containing UVs of selected glyphs stored in texture (key: glyphId, value: UV).
     * Because new lines have glyphId 0, we need to make sure we insert a default record with glyphId 0
     */
    std::unordered_map<uint32_t, GlyphInfo> _glyphs{{0, GlyphInfo{glm::vec2{0, 0}, glm::vec2{0, 0}}}};

public:
    FontAtlas(std::shared_ptr<Font> font, unsigned int fontSize, std::vector<uint32_t> glyphIds);
    FontAtlas(std::shared_ptr<Font> font, unsigned int fontSize, std::vector<CharacterRange> characterRanges);
    FontAtlas(std::shared_ptr<Font> font, unsigned int fontSize, std::u32string characters);
    FontAtlas(std::shared_ptr<Font> font, unsigned int fontSize);
    ~FontAtlas() = default;

    GlyphInfo getGlyph(uint32_t glyphId) const;

    std::string getFontFamily() const;
    glm::uvec2 getSize() const;
    const std::vector<uint8_t> &getTexture() const;

protected:
    std::vector<uint32_t> _getAllGlyphIds(std::shared_ptr<Font> font) const;
    std::vector<uint32_t> _getRangesGlyphIds(std::shared_ptr<Font> font,
                                             std::vector<CharacterRange> characterRanges) const;
    std::vector<uint32_t> _getUtf32GlyphIds(std::shared_ptr<Font> font, std::u32string characters) const;
};

}  // namespace vft