#include <p5cpp/graphics/pixels.hpp>
#include <p5cpp/graphics/pixel_ops.hpp>

#include <algorithm>
#include <cstddef>

namespace p5cpp
{
    Pixels makePixels(uint32_t width, uint32_t height)
    {
        return Pixels {width, height, std::vector<color_t>(static_cast<size_t>(width) * height)};
    }

    color_t get(const Pixels& pixels, uint32_t x, uint32_t y) { return pixels.data[static_cast<size_t>(y) * pixels.width + x]; }
    void set(Pixels& pixels, uint32_t x, uint32_t y, color_t color) { pixels.data[static_cast<size_t>(y) * pixels.width + x] = color; }
} // namespace p5cpp

namespace p5cpp::detail
{
    std::vector<color_t> flipRows(std::span<const color_t> src, uint32_t width, uint32_t height)
    {
        std::vector<color_t> flipped(src.size());
        for (uint32_t y = 0; y < height; ++y) {
            const size_t srcRow = static_cast<size_t>(height - 1 - y) * width;
            const size_t dstRow = static_cast<size_t>(y) * width;
            std::copy_n(src.begin() + static_cast<ptrdiff_t>(srcRow), width, flipped.begin() + static_cast<ptrdiff_t>(dstRow));
        }
        return flipped;
    }
} // namespace p5cpp::detail
