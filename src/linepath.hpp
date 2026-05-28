#pragma once

#include <vector>

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

        PathPoints buildDrawPoints(ColorStyle type);
        void vertex(float x, float y, float u, float v, color_t fillColor, color_t strokeColor);
        void clear();

    private:
        size_t drawPointCount;
        size_t drawPointCapacity;
        std::vector<float2> drawPointPositions;
        std::vector<float2> drawPointTexCoords;
        std::vector<color_t> drawPointFillColors;
        std::vector<color_t> drawPointStrokeColors;
    };
} // namespace p5
