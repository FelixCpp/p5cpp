#include "linepath.hpp"
#include "renderer.hpp"

#include <algorithm>

namespace p5
{
    LinePathBuilder::LinePathBuilder()
        : drawPointCount(0),
          drawPointCapacity(0),
          drawPointPositions(nullptr),
          drawPointTexCoords(nullptr),
          drawPointFillColors(nullptr),
          drawPointStrokeColors(nullptr)
    {
    }

    DrawPoints LinePathBuilder::buildDrawPoints(FillStyle type)
    {
        return DrawPoints {
            .size = drawPointCount,
            .positions = {drawPointPositions.get(), drawPointCount},
            .texcoords = {drawPointTexCoords.get(), drawPointCount},
            .colors = {type == FillStyle::fill ? drawPointFillColors.get() : drawPointStrokeColors.get(), drawPointCount},
        };
    }

    void LinePathBuilder::vertex(float x, float y, float u, float v, color_t fillColor, color_t strokeColor)
    {
        if (drawPointCount + 1 >= drawPointCapacity) {
            size_t newCapacity = std::max(drawPointCapacity * 2, static_cast<size_t>(4));
            auto newPositions = std::make_unique<float2[]>(newCapacity);
            auto newTexCoords = std::make_unique<float2[]>(newCapacity);
            auto newFillColors = std::make_unique<color_t[]>(newCapacity);
            auto newStrokeColors = std::make_unique<color_t[]>(newCapacity);

            for (size_t i = 0; i < drawPointCount; ++i) {
                newPositions[i] = drawPointPositions[i];
                newTexCoords[i] = drawPointTexCoords[i];
                newFillColors[i] = drawPointFillColors[i];
                newStrokeColors[i] = drawPointStrokeColors[i];
            }

            drawPointPositions = std::move(newPositions);
            drawPointTexCoords = std::move(newTexCoords);
            drawPointFillColors = std::move(newFillColors);
            drawPointStrokeColors = std::move(newStrokeColors);
            drawPointCapacity = newCapacity;
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
