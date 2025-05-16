/**
 * @file glyph_cache.h
 * @author Christian Saloň
 */

#pragma once

#include <algorithm>
#include <climits>
#include <cstdint>
#include <functional>
#include <list>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "glyph.h"

namespace vft {

/**
 * @brief Key for glyphs stored in glyph cache
 */
class GlyphKey {
public:
    std::string fontName;  /**< Font name of glyph */
    uint32_t glyphId;      /**< Glyph */
    unsigned int fontSize; /**< Font size of glyph */

    GlyphKey(std::string fontName, uint32_t glyphId, unsigned int fontSize);

    bool operator==(const GlyphKey &rhs) const = default;
};

/**
 * @brief Hash of glyph keys in cache
 */
struct GlyphKeyHash {
    std::size_t operator()(const GlyphKey &key) const {
        std::size_t fontNameHash = std::hash<std::string>()(key.fontName);
        std::size_t glyphIdHash = std::hash<uint32_t>()(key.glyphId);
        std::size_t fontSizeHash = std::hash<unsigned int>()(key.fontSize);
        return fontSizeHash ^ (glyphIdHash << 1) ^ (fontNameHash << 2);
    }
};

/**
 * @brief LRU cache for glyphs
 */
class GlyphCache {
private:
    unsigned long _maxSize{ULONG_MAX}; /**< Maximum size of cache */

    std::unordered_map<GlyphKey, Glyph, GlyphKeyHash> _cache{}; /**< Hash map used to store glyphs */
    std::list<GlyphKey> _used{}; /**< Linked list of glyphs in cache ordered by most recently used glyph */

public:
    GlyphCache(unsigned long maxSize);
    GlyphCache();
    ~GlyphCache() = default;

    void setGlyph(GlyphKey key, Glyph glyph);
    const Glyph &getGlyph(GlyphKey key);
    bool exists(GlyphKey key) const;

    void clearGlyph(GlyphKey key);
    void clearAll();
    void setMaxSize(unsigned long maxSize);

protected:
    void _eraseLRU();
    void _updateToMRU(GlyphKey key);
};

}  // namespace vft
