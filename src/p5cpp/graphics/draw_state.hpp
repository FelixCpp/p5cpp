#pragma once

#include <p5cpp/p5cpp.hpp>

#include <memory>

namespace p5
{
    struct DrawState
    {
        bool isFillEnabled = true;
        bool isStrokeEnabled = true;

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

        std::shared_ptr<Shader> shader = nullptr;
    };
} // namespace p5
