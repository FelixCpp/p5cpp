#pragma once

#include <p5cpp/p5cpp.hpp>

#include <optional>
#include <vector>

namespace p5
{
    namespace detail
    {
        struct ShapedLine
        {
            std::vector<ShapedGlyph> glyphs;
            float width;
        };

        struct LineLayout
        {
            std::vector<ShapedLine> lines;
            float unitsPerEm;
        };

        LineLayout layoutLines(const Font& font, float size, std::string_view str, TextWrap wrap, float maxWidth, float letterSpacing = 0.0f);

        struct TextBlockLayout
        {
            float2 blockOrigin;
            float blockTop;
            float leading;
            float blockWidth;
        };

        TextBlockLayout computeTextBlockLayout(const Font& font, const LineLayout& layout, float scale, TextAlignment alignment, float2 origin, std::optional<float> leadingOverride);

        float lineHorizontalOffset(float blockWidth, float lineWidthPixels, TextAlignment alignment);

        void appendLineToPoints(const Font& font, const ShapedLine& line, float scale, float penX, float penY, float letterSpacing, const TextToPointsOptions& options, std::vector<TextPoint>& outPoints, uint32_t& nextContourIndex);
    } // namespace detail
} // namespace p5
