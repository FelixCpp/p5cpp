#pragma once

#include "linepath.hpp"
#include "drawscope.hpp"

namespace p5
{
    struct Tesselator
    {
        virtual ~Tesselator() = default;
        virtual DrawScopeResult tesselate(DrawScope& scope, const PathPoints& points) = 0;
    };

    std::unique_ptr<Tesselator> createFanTesselator();
    std::unique_ptr<Tesselator> createConcaveTesselator();
} // namespace p5
