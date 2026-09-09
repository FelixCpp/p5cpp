#include <p5cpp/p5cpp.hpp>

namespace p5
{
    void Pixels::set(int32_t x, int32_t y, color_t color)
    {
        if (x < 0 or x >= static_cast<int32_t>(width) or y < 0 or y >= static_cast<int32_t>(height)) {
            error("Pixels::set(): coordinates out of bounds");
            return;
        }

        size_t index = static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x);
        size_t byteIndex = index * 4;

        data[byteIndex + 0] = getRed(color);
        data[byteIndex + 1] = getGreen(color);
        data[byteIndex + 2] = getBlue(color);
        data[byteIndex + 3] = getAlpha(color);
    }

    color_t Pixels::get(int32_t x, int32_t y) const
    {
        if (x < 0 or x >= static_cast<int32_t>(width) or y < 0 or y >= static_cast<int32_t>(height)) {
            error("Pixels::get(): coordinates out of bounds");
            return 0;
        }

        size_t index = static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x);
        size_t byteIndex = index * 4;

        uint8_t r = data.at(byteIndex + 0);
        uint8_t g = data.at(byteIndex + 1);
        uint8_t b = data.at(byteIndex + 2);
        uint8_t a = data.at(byteIndex + 3);

        return rgba(r, g, b, a);
    }

    ReadOnlyPixels Pixels::asReadOnly() const
    {
        return ReadOnlyPixels {
            .width = width,
            .height = height,
            .data = std::span<const uint8_t>(data.data(), data.size()),
        };
    }
} // namespace p5

namespace p5
{
    color_t ReadOnlyPixels::get(int32_t x, int32_t y) const
    {
        if (x < 0 or x >= static_cast<int32_t>(width) or y < 0 or y >= static_cast<int32_t>(height)) {
            error("ReadOnlyPixels::get(): coordinates out of bounds");
            return 0;
        }

        size_t index = static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x);
        size_t byteIndex = index * 4;

        uint8_t r = data[byteIndex + 0];
        uint8_t g = data[byteIndex + 1];
        uint8_t b = data[byteIndex + 2];
        uint8_t a = data[byteIndex + 3];

        return rgba(r, g, b, a);
    }
} // namespace p5
