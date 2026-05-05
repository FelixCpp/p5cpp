#pragma once

#include <memory>

#include "p5.hpp"
#include "renderer.hpp"

namespace p5
{
    class LinePathBuilder
    {
    public:
        LinePathBuilder();

        DrawPoints buildDrawPoints(FillStyle type);
        void vertex(float x, float y, float u, float v, color_t fillColor, color_t strokeColor);
        void clear();

    private:
        size_t drawPointCount;
        size_t drawPointCapacity;
        std::unique_ptr<float2[]> drawPointPositions;
        std::unique_ptr<float2[]> drawPointTexCoords;
        std::unique_ptr<color_t[]> drawPointFillColors;
        std::unique_ptr<color_t[]> drawPointStrokeColors;
    };
} // namespace p5
