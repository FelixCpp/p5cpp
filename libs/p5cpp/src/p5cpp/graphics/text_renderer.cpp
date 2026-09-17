#include <p5cpp/graphics/text_renderer.hpp>
#include <p5cpp/graphics/text_layout.hpp>

namespace p5
{
    namespace
    {
        constexpr size_t kTextMeshChunkGlyphs = 512;
    } // namespace

    TextRenderer::TextRenderer(GeometrySubmitter& submitter, const MatrixStack& matrixStack, Font defaultFont)
        : m_submitter(submitter),
          m_matrixStack(matrixStack),
          m_defaultFont(std::move(defaultFont))
    {
    }

    float2 TextRenderer::applyTransform(const float2& point) const
    {
        return p5::transformPoint(m_matrixStack.peek(), point);
    }

    void TextRenderer::text(const DrawState& state, std::string_view str, float x, float y, float maxWidth, float maxHeight)
    {
        const Font& font = state.textFont.isValid() ? state.textFont : m_defaultFont;
        const float scale = state.textSize / font.getUnitsPerEm();

        const detail::LineLayout layout = detail::layoutLines(font, state.textSize, str, state.textWrap, maxWidth, state.textLetterSpacing, state.textLigatures);
        const size_t numLines = layout.lines.size();

        const detail::TextBlockLayout blockLayout = detail::computeTextBlockLayout(font, layout, scale, state.textAlignment, {x, y}, state.textLeadingOverride);
        const float leading = blockLayout.leading;
        const float blockTop = blockLayout.blockTop;
        const float2 blockOrigin = blockLayout.blockOrigin;
        const float blockWidth = blockLayout.blockWidth;

        size_t visibleLines = numLines;
        if (maxHeight > 0.0f) {
            visibleLines = 0;
            for (size_t i = 0; i < numLines; ++i) {
                const float baselineOffset = blockTop + static_cast<float>(i) * leading;
                if (baselineOffset > maxHeight and visibleLines > 0) {
                    break;
                }
                ++visibleLines;
            }
        }

        std::vector<float2> positions;
        std::vector<float2> texCoords;
        std::vector<color_t> colors;
        size_t pendingGlyphs = 0;

        const auto flushGlyphChunk = [&]() {
            if (not positions.empty()) {
                m_submitter.submitTextMesh(positions, texCoords, colors, font.getAtlasTexture(), state);
                positions.clear();
                texCoords.clear();
                colors.clear();
                pendingGlyphs = 0;
            }
        };

        for (size_t lineIndex = 0; lineIndex < visibleLines; ++lineIndex) {
            const detail::ShapedLine& line = layout.lines[lineIndex];
            const float lineWidthPixels = line.width * scale;
            float penX = blockOrigin.x + detail::lineHorizontalOffset(blockWidth, lineWidthPixels, state.textAlignment);
            const float penYBaseline = blockOrigin.y + blockTop + static_cast<float>(lineIndex) * leading;
            float penY = penYBaseline;

            for (const ShapedGlyph& g : line.glyphs) {
                const GlyphMetrics& metrics = font.getGlyphMetrics(g.glyphIndex);
                if (metrics.hasOutline) {
                    const float2 glyphOrigin {penX + g.xOffset * scale, penY - g.yOffset * scale};
                    const float2 quadTopLeft {glyphOrigin.x + metrics.bounds.left * scale, glyphOrigin.y - metrics.bounds.top * scale};
                    const float2 quadSize {metrics.bounds.width * scale, metrics.bounds.height * scale};

                    const float2 corners[4] = {
                        applyTransform({quadTopLeft.x, quadTopLeft.y}),
                        applyTransform({quadTopLeft.x + quadSize.x, quadTopLeft.y}),
                        applyTransform({quadTopLeft.x + quadSize.x, quadTopLeft.y + quadSize.y}),
                        applyTransform({quadTopLeft.x, quadTopLeft.y + quadSize.y}),
                    };
                    const float2 uv[4] = {
                        {metrics.uvRect.left, metrics.uvRect.top},
                        {metrics.uvRect.left + metrics.uvRect.width, metrics.uvRect.top},
                        {metrics.uvRect.left + metrics.uvRect.width, metrics.uvRect.top + metrics.uvRect.height},
                        {metrics.uvRect.left, metrics.uvRect.top + metrics.uvRect.height},
                    };

                    positions.insert(positions.end(), std::begin(corners), std::end(corners));
                    texCoords.insert(texCoords.end(), std::begin(uv), std::end(uv));
                    colors.insert(colors.end(), 4, state.fillColor);

                    if (++pendingGlyphs >= kTextMeshChunkGlyphs) {
                        flushGlyphChunk();
                    }
                }

                penX += g.xAdvance * scale + state.textLetterSpacing;
                penY -= g.yAdvance * scale;
            }
        }

        flushGlyphChunk();
    }

    void TextRenderer::text(const DrawState& state, std::u32string_view str, float x, float y, float maxWidth, float maxHeight)
    {
        text(state, detail::utf32ToUtf8(str), x, y, maxWidth, maxHeight);
    }

    float TextRenderer::textWidth(const DrawState& state, std::string_view str)
    {
        const Font& font = state.textFont.isValid() ? state.textFont : m_defaultFont;
        return p5::textWidth(font, state.textSize, str, state.textLetterSpacing, state.textLigatures);
    }

    float TextRenderer::textWidth(const DrawState& state, std::u32string_view str)
    {
        return textWidth(state, detail::utf32ToUtf8(str));
    }

    rect2f TextRenderer::textBounds(const DrawState& state, std::string_view str, const TextBoundsOptions& options)
    {
        const Font& font = options.font.value_or(state.textFont.isValid() ? state.textFont : m_defaultFont);
        const float size = options.size.value_or(state.textSize);
        const float letterSpacing = options.letterSpacing.value_or(state.textLetterSpacing);
        const TextWrap wrap = options.wrap.value_or(state.textWrap);
        const TextAlignment alignment = options.alignment.value_or(state.textAlignment);
        const std::optional<float> leadingOverride = options.leading.has_value() ? options.leading : state.textLeadingOverride;
        const bool ligaturesEnabled = options.ligatures.value_or(state.textLigatures);

        const float scale = size / font.getUnitsPerEm();
        const detail::LineLayout layout = detail::layoutLines(font, size, str, wrap, options.maxWidth, letterSpacing, ligaturesEnabled);
        const detail::TextBlockLayout blockLayout = detail::computeTextBlockLayout(font, layout, scale, alignment, {0.0f, 0.0f}, leadingOverride);

        size_t visibleLines = layout.lines.size();
        if (options.maxHeight > 0.0f) {
            visibleLines = 0;
            for (size_t i = 0; i < layout.lines.size(); ++i) {
                const float baselineOffset = blockLayout.blockTop + static_cast<float>(i) * blockLayout.leading;
                if (baselineOffset > options.maxHeight and visibleLines > 0) {
                    break;
                }
                ++visibleLines;
            }
        }

        const float blockHeight = blockLayout.blockTop + static_cast<float>(visibleLines > 0 ? visibleLines - 1 : 0) * blockLayout.leading + font.getDescent() * scale;

        return rect2f {blockLayout.blockOrigin.x, blockLayout.blockOrigin.y, blockLayout.blockWidth, blockHeight};
    }

    rect2f TextRenderer::textBounds(const DrawState& state, std::u32string_view str, const TextBoundsOptions& options)
    {
        return textBounds(state, detail::utf32ToUtf8(str), options);
    }

    std::vector<TextPoint> TextRenderer::textToPoints(const DrawState& state, std::string_view str, float x, float y, const TextToPointsOptions& options)
    {
        const Font& font = options.font.has_value() ? *options.font : (state.textFont.isValid() ? state.textFont : m_defaultFont);
        const float effectiveSize = options.size.value_or(state.textSize);
        const float effectiveLetterSpacing = options.letterSpacing.value_or(state.textLetterSpacing);
        const bool effectiveLigatures = options.ligatures.value_or(state.textLigatures);
        const float scale = effectiveSize / font.getUnitsPerEm();

        const detail::LineLayout layout = detail::layoutLines(font, effectiveSize, str, TextWrap::none, 0.0f, effectiveLetterSpacing, effectiveLigatures);
        const detail::TextBlockLayout blockLayout = detail::computeTextBlockLayout(font, layout, scale, state.textAlignment, {x, y}, state.textLeadingOverride);

        std::vector<TextPoint> result;
        uint32_t nextContourIndex = 0;
        for (size_t lineIndex = 0; lineIndex < layout.lines.size(); ++lineIndex) {
            const detail::ShapedLine& line = layout.lines[lineIndex];
            const float lineWidthPixels = line.width * scale;
            const float penX = blockLayout.blockOrigin.x + detail::lineHorizontalOffset(blockLayout.blockWidth, lineWidthPixels, state.textAlignment);
            const float penY = blockLayout.blockOrigin.y + blockLayout.blockTop + static_cast<float>(lineIndex) * blockLayout.leading;
            detail::appendLineToPoints(font, line, scale, penX, penY, effectiveLetterSpacing, options, result, nextContourIndex);
        }

        return result;
    }

    std::vector<TextPoint> TextRenderer::textToPoints(const DrawState& state, std::u32string_view str, float x, float y, const TextToPointsOptions& options)
    {
        return textToPoints(state, detail::utf32ToUtf8(str), x, y, options);
    }
} // namespace p5
