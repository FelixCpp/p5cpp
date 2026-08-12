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
        color_t strokeColor = rgba(0, 0, 0);
        float strokeWeight = 1.0f;

        StrokeCap strokeCap = StrokeCap::round;
        StrokeJoin strokeJoin = StrokeJoin::miter;
        float strokeMiterLimit = 10.0f;
        float strokeRoundJoinThreshold = 0.3f;

        BlendMode blendMode = BlendMode::alpha;

        std::shared_ptr<Shader> shader = nullptr;
    };
} // namespace p5
