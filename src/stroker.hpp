#pragma once

#include "p5.hpp"
#include "drawscope.hpp"
#include "renderer.hpp"

#include <memory>

namespace p5
{
    struct Stroker
    {
        virtual ~Stroker() = default;
        virtual void stroke(DrawScope& scope, const DrawPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinAngleThresold, bool close) = 0;
    };

    std::unique_ptr<Stroker> createStroker();
} // namespace p5
