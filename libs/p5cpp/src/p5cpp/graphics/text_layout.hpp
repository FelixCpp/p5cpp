#pragma once

#include <p5cpp/p5cpp.hpp>

#include <optional>
#include <vector>

namespace p5
{
    namespace detail
    {
        // One line's worth of already-shaped glyphs (design units, unscaled) produced by layoutLines().
        struct ShapedLine
        {
            std::vector<ShapedGlyph> glyphs;
            float width; // sum of glyphs' xAdvance plus per-glyph letter spacing, design units
        };

        struct LineLayout
        {
            std::vector<ShapedLine> lines;
            float unitsPerEm;
        };

        // Splits `str` into laid-out lines per `wrap`/`maxWidth` (pixels; maxWidth is ignored when wrap
        // is TextWrap::none). letterSpacing is in pixels (extra space appended after every glyph,
        // including the last) and factors into both the wrap-point decisions and the returned
        // ShapedLine::width, so it can never disagree between measurement and drawing.
        LineLayout layoutLines(const Font& font, float size, std::string_view str, TextWrap wrap, float maxWidth, float letterSpacing = 0.0f);

        // Block-level placement shared by Graphics::text() and Graphics::textToPoints(): where the
        // whole (possibly multi-line) text block starts, and how far down each subsequent line's
        // baseline sits, given `alignment` and the pixel-space `origin` the caller was placed at.
        struct TextBlockLayout
        {
            float2 blockOrigin; // top-left of the text block after alignment offsets, pixels
            float blockTop;     // ascent*scale -- distance from blockOrigin.y down to line 0's baseline
            float leading;      // pixels between consecutive lines' baselines
            float blockWidth;   // widest line's pixel width -- pass to lineHorizontalOffset() per line
        };

        TextBlockLayout computeTextBlockLayout(const Font& font, const LineLayout& layout, float scale, TextAlignment alignment, float2 origin, std::optional<float> leadingOverride);

        // Horizontal offset (pixels) to add to a line's pen start so a `lineWidthPixels`-wide line ends
        // up aligned within a `blockWidth`-wide block per `alignment`.
        float lineHorizontalOffset(float blockWidth, float lineWidthPixels, TextAlignment alignment);

        // Used by Graphics::textToPoints(): walks one already-shaped line's glyphs starting at pen
        // position (penX, penY) -- glyph 0's baseline -- advancing per glyph exactly like
        // Graphics::text()'s pen math (xAdvance/yAdvance*scale + letterSpacing), and appends TextPoints
        // sampled along every glyph's outline contours (per `options`) to `outPoints`, in glyph/contour
        // order. The caller owns line-to-line advancement (leading) and per-line horizontal alignment.
        // Font& (not const) because it calls Font::getGlyphContours(), which lazily caches per-glyph data.
        void appendLineToPoints(Font& font, const ShapedLine& line, float scale, float penX, float penY, float letterSpacing, const TextToPointsOptions& options, std::vector<TextPoint>& outPoints);
    } // namespace detail
} // namespace p5
