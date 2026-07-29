#pragma once

#include <p5cpp/graphics/color.hpp>
#include <p5cpp/math/value2.hpp>

#include <vector>

namespace p5cpp
{
    // Top-left origin, row-major pixel buffer — matches the coordinate system used
    // everywhere else in p5cpp (e.g. rect()/image()), unlike Framebuffer::readPixels()/
    // writePixels() which use raw bottom-to-top GL row order.
    class Pixels
    {
    public:
        Pixels();
        Pixels(uint32_t width, uint32_t height);
        Pixels(uint32_t width, uint32_t height, std::vector<color_t> data);

        uint2 getSize() const;
        uint32_t getWidth() const;
        uint32_t getHeight() const;
        size_t size() const;

        color_t& operator[](size_t index);
        color_t operator[](size_t index) const;

        color_t get(uint32_t x, uint32_t y) const;
        void set(uint32_t x, uint32_t y, color_t color);

        color_t* data();
        const color_t* data() const;

        std::vector<color_t>::iterator begin();
        std::vector<color_t>::iterator end();
        std::vector<color_t>::const_iterator begin() const;
        std::vector<color_t>::const_iterator end() const;

    private:
        uint32_t m_width;
        uint32_t m_height;
        std::vector<color_t> m_data;
    };
} // namespace p5cpp
