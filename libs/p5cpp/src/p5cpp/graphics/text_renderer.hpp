#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/draw_state.hpp>
#include <p5cpp/graphics/matrix_stack.hpp>
#include <p5cpp/graphics/geometry_submitter.hpp>

namespace p5
{
    class TextRenderer
    {
    public:
        TextRenderer(GeometrySubmitter& submitter, const MatrixStack& matrixStack, Font defaultFont);

        void text(const DrawState& state, std::string_view str, float x, float y, float maxWidth, float maxHeight);
        void text(const DrawState& state, std::u32string_view str, float x, float y, float maxWidth, float maxHeight);

        float textWidth(const DrawState& state, std::string_view str);
        float textWidth(const DrawState& state, std::u32string_view str);

        rect2f textBounds(const DrawState& state, std::string_view str, const TextBoundsOptions& options);
        rect2f textBounds(const DrawState& state, std::u32string_view str, const TextBoundsOptions& options);

        std::vector<TextPoint> textToPoints(const DrawState& state, std::string_view str, float x, float y, const TextToPointsOptions& options);
        std::vector<TextPoint> textToPoints(const DrawState& state, std::u32string_view str, float x, float y, const TextToPointsOptions& options);

    private:
        float2 applyTransform(const float2& point) const;

        GeometrySubmitter& m_submitter;
        const MatrixStack& m_matrixStack;
        Font m_defaultFont;
    };
} // namespace p5
