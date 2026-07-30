#pragma once

#include <p5cpp/graphics/color.hpp>
#include <p5cpp/math/value2.hpp>

#include <cstdint>
#include <vector>

namespace p5cpp
{
    // Top-left origin, row-major pixel buffer — matches the coordinate system used
    // everywhere else in p5cpp (e.g. rect()/image()), unlike Framebuffer's readPixels()/
    // writePixels() which use raw bottom-to-top GL row order. Plain data - no shared
    // ownership, no GPU/OS resource behind it, just a std::vector<color_t> with its
    // dimensions.
    struct Pixels
    {
        uint32_t width = 0;
        uint32_t height = 0;
        std::vector<color_t> data;
    };

    inline uint2 getSize(const Pixels& pixels) { return uint2(pixels.width, pixels.height); }

    // Zero-initialized width*height buffer, ready for get()/set().
    Pixels makePixels(uint32_t width, uint32_t height);

    color_t get(const Pixels& pixels, uint32_t x, uint32_t y);
    void set(Pixels& pixels, uint32_t x, uint32_t y, color_t color);
} // namespace p5cpp
