#pragma once

#include <memory>

#include "p5.hpp"
#include "vertex.hpp"

namespace p5
{
    struct PathPoints
    {
        size_t size;
        std::span<const float2> positions;
        std::span<const float2> texcoords;
        std::span<const color_t> colors;
    };

    class LinePathBuilder
    {
    public:
        LinePathBuilder();

        PathPoints buildDrawPoints(FillStyle type);
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
