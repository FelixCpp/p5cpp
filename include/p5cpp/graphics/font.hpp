#pragma once

#include <p5cpp/graphics/texture.hpp>
#include <p5cpp/graphics/text.hpp>

#include <p5cpp/math/value2.hpp>
#include <p5cpp/math/rectangle.hpp>

#include <memory>
#include <span>
#include <vector>
#include <filesystem>

namespace p5cpp
{
    typedef size_t GlyphPageIndex;

    struct GlyphRegion
    {
        int2 size;
        float_rect uvRect;
    };

    struct Glyph
    {
        GlyphRegion region;
        int2 bearing;
        float advanceX;
        size_t glyphAtlasIndex;
    };
} // namespace p5cpp

namespace p5cpp
{
    struct FontMetrics
    {
        float ascender;
        float descender;
        float lineHeight;
    };
} // namespace p5cpp

namespace p5cpp
{
    // A single closed contour of a glyph outline — either an outer boundary or an inner hole
    // (e.g. the counter of an "O"). Points are not repeated at the end; the contour is implicitly
    // closed back to points[0].
    using TextContour = std::vector<float2>;
} // namespace p5cpp

namespace p5cpp
{
    struct ShapedGlyph
    {
        int2 bearing;
        int2 size;
        float_rect uvRect;
        size_t glyphAtlasIndex;
        float xOffset;      // HarfBuzz cluster-relative offset in pixels
        float yOffset;      // HarfBuzz cluster-relative offset in pixels
        float xAdvance;     // HarfBuzz advance in pixels
        float yAdvance;     // HarfBuzz advance in pixels
        bool isWhitespace;  // true for space/tab — still advances but has no bitmap
        char32_t cluster;   // leading codepoint of the cluster (for word-wrap)
    };
} // namespace p5cpp

namespace p5cpp
{
    namespace detail
    {
        struct FontResource;
    }

    struct Font;

    // See isFontValid() — returns an invalid Font on parse failure. loadFont(path)
    // caches by path (repeated calls for the same path return a Font that aliases the
    // same underlying FreeType/HarfBuzz state instead of re-parsing); loadFont(span) is
    // uncached, since there's no stable key to cache by.
    Font loadFont(const std::filesystem::path& fontFilePath);
    Font loadFont(std::span<const uint8_t> fontData);

    // A loaded font handle (FreeType parsing + HarfBuzz shaping + a dynamically-growing
    // glyph atlas). Copies are cheap and alias the same underlying state (shared_ptr-
    // backed); everything is torn down automatically once the last copy is destroyed.
    // Default-constructed instances are "invalid" - see isFontValid().
    struct Font
    {
        // Internal handle driving automatic cleanup - not meant to be read or written
        // directly. Public (rather than private+friend) because detail::FontResource
        // is opaque outside font.cpp: exposing the pointer can't be used to fabricate
        // a working Font, only to alias or null out this one.
        std::shared_ptr<detail::FontResource> resource;
    };

    bool isFontValid(const Font& font);

    const Glyph* getGlyph(const Font& font, char32_t codepoint, int textSize);
    const FontMetrics* getMetrics(const Font& font, int textSize);
    float getKerning(const Font& font, char32_t leftCodepoint, char32_t rightCodepoint, int textSize);
    const Texture* getGlyphAtlasTexture(const Font& font, size_t glyphAtlasIndex);
    std::vector<ShapedGlyph> shape(const Font& font, std::string_view text, int textSize);

    // Returns the outline of `text` as a list of closed contours (one per letter part —
    // e.g. "O" yields two: the outer boundary and the inner hole), in the same coordinate
    // space as the (x, y) baseline-left origin passed here. '\n' starts a new line, advanced
    // by this font's line height at `textSize`. `curveDetail` controls how many line segments
    // each curve in the glyph outline is flattened into. If `spacing` is greater than 0, every
    // contour is additionally resampled to points evenly spaced `spacing` pixels apart along
    // its arc length (matching p5.js's textToPoints()); otherwise contours keep the raw,
    // curvature-biased points produced by the outline decomposition. If `maxWidth` is greater
    // than 0 and `wrap` is not TextWrap::none, text additionally wraps the same way as
    // text(text, x, y, maxWidth) does.
    std::vector<TextContour> textToPoints(const Font& font, std::string_view text, float x, float y, int textSize, int curveDetail = 8, float spacing = 0.0f, float maxWidth = -1.0f, TextWrap wrap = TextWrap::none);
} // namespace p5cpp
