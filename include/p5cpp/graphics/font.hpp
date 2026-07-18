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
    struct FontImpl
    {
        virtual ~FontImpl() = default;
        virtual const Glyph* getGlyph(char32_t codepoint, int textSize) = 0;
        virtual const FontMetrics* getMetrics(int textSize) = 0;
        virtual float getKerning(char32_t leftCodepoint, char32_t rightCodepoint, int textSize) = 0;
        virtual const Texture* getGlyphAtlasTexture(size_t glyphAtlasIndex) = 0;
        virtual std::vector<ShapedGlyph> shape(std::string_view text, int textSize) = 0;
        virtual std::vector<TextContour> textToPoints(std::string_view text, float x, float y, int textSize, int curveDetail, float maxWidth, TextWrap wrap) = 0;
    };

    std::unique_ptr<FontImpl> loadFont(const std::filesystem::path& fontFilePath);
    std::unique_ptr<FontImpl> loadFont(std::span<const uint8_t> fontData);
} // namespace p5cpp

namespace p5cpp
{
    class Font
    {
    public:
        Font();
        Font(std::unique_ptr<FontImpl> impl);
        Font(std::shared_ptr<FontImpl> impl);

        const Glyph* getGlyph(char32_t codepoint, int textSize) const;
        const FontMetrics* getMetrics(int textSize) const;
        float getKerning(char32_t leftCodepoint, char32_t rightCodepoint, int textSize) const;
        const Texture* getGlyphAtlasTexture(size_t glyphAtlasIndex) const;
        std::vector<ShapedGlyph> shape(std::string_view text, int textSize) const;

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
        std::vector<TextContour> textToPoints(std::string_view text, float x, float y, int textSize, int curveDetail = 8, float spacing = 0.0f, float maxWidth = -1.0f, TextWrap wrap = TextWrap::none) const;

    private:
        std::shared_ptr<FontImpl> impl;
    };
} // namespace p5cpp
