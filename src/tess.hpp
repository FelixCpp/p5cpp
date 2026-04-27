#pragma once

#include "renderer.hpp"
#include "drawscope.hpp"

namespace p5
{
    struct Tesselator
    {
        virtual ~Tesselator() = default;
        virtual void fill(DrawScope& scope, const DrawPoints& points) = 0;
        virtual void stroke(DrawScope& scope, const DrawPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, StrokeAlign strokeAlign, float miterLimit, bool close) = 0;
    };

    std::unique_ptr<Tesselator> createTesselator();
} // namespace p5
