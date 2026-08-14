#pragma once

#include <p5cpp/p5cpp.hpp>

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
    } // namespace detail
} // namespace p5
