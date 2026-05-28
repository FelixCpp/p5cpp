#include "linepath.hpp"

#include <algorithm>

namespace p5
{
    LinePathBuilder::LinePathBuilder()
        : drawPointCount(0),
          drawPointCapacity(0)
    {
    }

    PathPoints LinePathBuilder::buildDrawPoints(ColorStyle type)
    {
        return PathPoints {
            .size = drawPointCount,
            .positions = drawPointPositions,
            .texcoords = drawPointTexCoords,
            .colors = type == ColorStyle::fill ? drawPointFillColors : drawPointStrokeColors,
        };
    }

    void LinePathBuilder::vertex(float x, float y, float u, float v, color_t fillColor, color_t strokeColor)
    {
        if (drawPointCount + 1 >= drawPointCapacity) {
            drawPointCapacity = std::max(drawPointCapacity * 2, static_cast<size_t>(4));
            drawPointPositions.resize(drawPointCapacity);
            drawPointTexCoords.resize(drawPointCapacity);
            drawPointFillColors.resize(drawPointCapacity);
            drawPointStrokeColors.resize(drawPointCapacity);
        }

        drawPointPositions[drawPointCount] = {x, y};
        drawPointTexCoords[drawPointCount] = {u, v};
        drawPointFillColors[drawPointCount] = fillColor;
        drawPointStrokeColors[drawPointCount] = strokeColor;
        drawPointCount++;
    }

    void LinePathBuilder::clear()
    {
        drawPointCount = 0;
    }
} // namespace p5
