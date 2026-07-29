#pragma once

#include <p5cpp/graphics/color.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace p5cpp::detail
{
    // Not part of the public API (this header lives under src/, not include/p5cpp/,
    // and is never pulled in by p5cpp.hpp). Reverses row order of a width*height color
    // buffer - converts between OpenGL's bottom-to-top texture/framebuffer row order
    // and Pixels' top-left-origin convention (used by both directions: reading GL data
    // into a Pixels, and writing a Pixels' data back via Texture::upload()/
    // Framebuffer::writePixels()). Self-inverse: applying it twice returns the
    // original row order.
    std::vector<color_t> flipRows(std::span<const color_t> src, uint32_t width, uint32_t height);
} // namespace p5cpp::detail
