#pragma once

#include <p5cpp/p5cpp.hpp>

#include <memory>
#include <optional>

namespace p5
{
    struct DrawState
    {
        bool isFillEnabled = true;
        bool isStrokeEnabled = true;

        std::optional<rect2f> clipRect = std::nullopt;

        color_t fillColor = rgba(255, 255, 255);
        color_t strokeColor = rgba(255, 255, 255);
        color_t tintColor = rgba(255, 255, 255);
        float strokeWeight = 1.0f;

        StrokeCap strokeCap = StrokeCap::round;
        StrokeJoin strokeJoin = StrokeJoin::miter;
        float strokeMiterLimit = 10.0f;
        float strokeRoundJoinThreshold = radians(10.0f);
        float curveTightness = 0.0f;

        BlendMode blendMode = BlendMode::alpha;
        TextureUVMode textureUVMode = TextureUVMode::normalized;
        TextureFilter textureFilter = TextureFilter::linear;
        TextureWrap textureWrap = TextureWrap::clampToEdge;

        Shader shader;

        Font textFont;
        float textSize = 12.0f;
        TextAlignment textAlignment = TextAlignment::topLeft;
        TextWrap textWrap = TextWrap::none;
        std::optional<float> textLeadingOverride = std::nullopt;
        float textLetterSpacing = 0.0f;
    };
} // namespace p5
