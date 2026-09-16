#include <p5cpp/p5cpp.hpp>

#include <cstring>

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

        data[byteIndex + 0] = color.r;
        data[byteIndex + 1] = color.g;
        data[byteIndex + 2] = color.b;
        data[byteIndex + 3] = color.a;
    }

    color_t Pixels::get(int32_t x, int32_t y) const
    {
        if (x < 0 or x >= static_cast<int32_t>(width) or y < 0 or y >= static_cast<int32_t>(height)) {
            error("Pixels::get(): coordinates out of bounds");
            return {};
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

    Pixels Pixels::getSubPixels(uint32_t x, uint32_t y, uint32_t subWidth, uint32_t subHeight) const
    {
        return asReadOnly().getSubPixels(x, y, subWidth, subHeight);
    }
} // namespace p5

namespace p5
{
    color_t ReadOnlyPixels::get(int32_t x, int32_t y) const
    {
        if (x < 0 or x >= static_cast<int32_t>(width) or y < 0 or y >= static_cast<int32_t>(height)) {
            error("ReadOnlyPixels::get(): coordinates out of bounds");
            return {};
        }

        size_t index = static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x);
        size_t byteIndex = index * 4;

        uint8_t r = data[byteIndex + 0];
        uint8_t g = data[byteIndex + 1];
        uint8_t b = data[byteIndex + 2];
        uint8_t a = data[byteIndex + 3];

        return rgba(r, g, b, a);
    }

    Pixels ReadOnlyPixels::getSubPixels(uint32_t x, uint32_t y, uint32_t subWidth, uint32_t subHeight) const
    {
        if (subWidth == 0 or subHeight == 0) {
            error("getSubPixels() region must have non-zero width and height");
            return {};
        }
        if (x + subWidth > width or y + subHeight > height) {
            error("getSubPixels() region is out of bounds");
            return {};
        }

        Pixels result;
        result.width = subWidth;
        result.height = subHeight;
        result.data.resize(static_cast<size_t>(subWidth) * static_cast<size_t>(subHeight) * 4);

        const size_t rowBytes = static_cast<size_t>(subWidth) * 4;
        for (uint32_t row = 0; row < subHeight; ++row) {
            const uint8_t* src = data.data() + (static_cast<size_t>(y + row) * width + x) * 4;
            uint8_t* dst = result.data.data() + static_cast<size_t>(row) * rowBytes;
            std::memcpy(dst, src, rowBytes);
        }

        return result;
    }
} // namespace p5
