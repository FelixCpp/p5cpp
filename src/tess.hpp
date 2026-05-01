#pragma once

#include "renderer.hpp"
#include "drawscope.hpp"

namespace p5
{
    struct Tesselator
    {
        virtual ~Tesselator() = default;
        virtual void tesselate(DrawScope& scope, const DrawPoints& points) = 0;
    };

    std::unique_ptr<Tesselator> createTesselator();
} // namespace p5
